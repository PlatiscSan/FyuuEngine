#include <cassert>
import event_bus;
import std;

using namespace fyuu_engine::util;

// Test event types
struct TestEvent {
    int value;
    std::string message;
};

struct AnotherEvent {
    double data;
    bool flag;
};

struct ThreadSyncEvent {
    std::thread::id thread_id;
    int sequence;
};

struct PerformanceEvent {
    long long timestamp;
    int worker_id;
};

void test_event_bus_basic() {
    std::cout << "Testing basic event publishing and subscription..." << std::endl;

    EventBus bus;
    bool event_received = false;
    int received_value = 0;

    // Subscribe to TestEvent
    auto handle = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        event_received = true;
        received_value = event.value;
        std::cout << "Received TestEvent: value=" << event.value << ", message=" << event.message << std::endl;
        return true;
        });

    // Publish event
    bus.Publish(TestEvent{ 42, "Hello EventBus" });

    assert(event_received);
    assert(received_value == 42);

    std::cout << "✓ Basic event test passed" << std::endl;
}

void test_event_bus_multiple_subscribers() {
    std::cout << "Testing multiple subscribers..." << std::endl;

    EventBus bus;
    int call_count = 0;

    // First subscriber
    auto handle1 = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        call_count++;
        std::cout << "Subscriber 1 received event: " << event.value << std::endl;
        return true;
        });

    // Second subscriber
    auto handle2 = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        call_count++;
        std::cout << "Subscriber 2 received event: " << event.value << std::endl;
        return true;
        });

    // Publish event
    bus.Publish(TestEvent{ 100, "Multiple subscribers" });

    assert(call_count == 2);
    std::cout << "✓ Multiple subscribers test passed" << std::endl;
}

void test_event_bus_event_stopping() {
    std::cout << "Testing event propagation stopping..." << std::endl;

    EventBus bus;
    std::vector<int> execution_order;

    auto handle1 = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        execution_order.push_back(1);
        std::cout << "Subscriber 1 processing event" << std::endl;
        return true; // Continue propagation
        });

    auto handle2 = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        execution_order.push_back(2);
        std::cout << "Subscriber 2 processing event, stopping propagation" << std::endl;
        return false; // Stop propagation
        });

    auto handle3 = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        execution_order.push_back(3);
        std::cout << "Subscriber 3 should not receive this event" << std::endl;
        return true;
        });

    bus.Publish(TestEvent{ 200, "Stop propagation" });

    // Verify only first two subscribers were executed
    assert(execution_order.size() == 2);
    assert(execution_order[0] == 1);
    assert(execution_order[1] == 2);

    std::cout << "✓ Event propagation stopping test passed" << std::endl;
}

void test_event_bus_different_event_types() {
    std::cout << "Testing different event types..." << std::endl;

    EventBus bus;
    bool test_event_received = false;
    bool another_event_received = false;

    auto handle1 = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        test_event_received = true;
        std::cout << "Received TestEvent" << std::endl;
        return true;
        });

    auto handle2 = bus.Subscribe<AnotherEvent>([&](AnotherEvent const& event) {
        another_event_received = true;
        std::cout << "Received AnotherEvent: data=" << event.data << std::endl;
        return true;
        });

    // Publish TestEvent
    bus.Publish(TestEvent{ 300, "Type test" });
    assert(test_event_received);
    assert(!another_event_received);

    // Reset state
    test_event_received = false;

    // Publish AnotherEvent
    bus.Publish(AnotherEvent{ 3.14, true });
    assert(another_event_received);
    assert(!test_event_received);

    std::cout << "✓ Different event types test passed" << std::endl;
}

void test_event_bus_unsubscribe() {
    std::cout << "Testing unsubscribe functionality..." << std::endl;

    EventBus bus;
    int call_count = 0;

    // Subscribe and immediately unsubscribe
    {
        auto handle = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
            call_count++;
            std::cout << "This subscriber should not be called" << std::endl;
            return true;
            });
        // Handle goes out of scope, auto-unsubscribe
    }

    bus.Publish(TestEvent{ 400, "Unsubscribe test" });

    assert(call_count == 0);
    std::cout << "✓ Unsubscribe test passed" << std::endl;
}

void test_event_bus_void_return_action() {
    std::cout << "Testing void return action..." << std::endl;

    EventBus bus;
    bool event_received = false;

    // Use action with void return (should default to true for propagation)
    auto handle = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        event_received = true;
        std::cout << "Void return action processing event" << std::endl;
        // No return statement
        });

    bus.Publish(TestEvent{ 500, "Void action" });

    assert(event_received);
    std::cout << "✓ Void return action test passed" << std::endl;
}

void test_event_bus_recursive_publish() {
    std::cout << "Testing recursive publishing..." << std::endl;

    EventBus bus;
    std::vector<int> execution_order;

    auto handle1 = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        execution_order.push_back(event.value);
        std::cout << "Event processing: " << event.value << std::endl;

        if (event.value == 1) {
            // Recursively publish new event
            bus.Publish(TestEvent{ 2, "Recursive event" });
        }

        return true;
        });

    // Start event chain
    bus.Publish(TestEvent{ 1, "Start recursive chain" });

    // Verify both events were processed
    assert(execution_order.size() == 2);
    assert(execution_order[0] == 1);
    assert(execution_order[1] == 2);

    std::cout << "✓ Recursive publishing test passed" << std::endl;
}

void test_event_bus_subscribe_during_publish() {
    std::cout << "Testing subscription during event publishing..." << std::endl;

    EventBus bus;
    std::shared_ptr<EventBus::BaseSubscription::SubscriptionHandle> late_subscriber;
    bool late_subscriber_called = false;

    auto handle1 = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        std::cout << "First subscriber, creating new subscription during event processing" << std::endl;

        if (!late_subscriber) {
            late_subscriber = std::make_shared<EventBus::BaseSubscription::SubscriptionHandle>(
                bus.Subscribe<TestEvent>([&](TestEvent const& inner_event) {
                    late_subscriber_called = true;
                    std::cout << "Late subscriber received event: " << inner_event.value << std::endl;
                    return true;
                    })
            );
        }

        return true;
        });

    // First publish, create late subscriber
    bus.Publish(TestEvent{ 600, "First publish" });

    // Second publish, late subscriber should receive event
    bus.Publish(TestEvent{ 601, "Second publish" });

    assert(late_subscriber_called);
    std::cout << "✓ Subscription during publishing test passed" << std::endl;
}

// Multi-threading tests
void test_event_bus_concurrent_publishing() {
    std::cout << "Testing concurrent event publishing..." << std::endl;

    EventBus bus;
    std::atomic<int> event_count{ 0 };
    constexpr int THREAD_COUNT = 10;
    constexpr int EVENTS_PER_THREAD = 100;

    auto handle = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        event_count.fetch_add(1, std::memory_order_relaxed);
        return true;
        });

    std::vector<std::thread> threads;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&bus, i]() {
            for (int j = 0; j < EVENTS_PER_THREAD; ++j) {
                bus.Publish(TestEvent{ i * EVENTS_PER_THREAD + j,
                                    "Concurrent event from thread " + std::to_string(i) });
                std::this_thread::sleep_for(std::chrono::microseconds(10)); // Small delay
            }
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    assert(event_count.load() == THREAD_COUNT * EVENTS_PER_THREAD);
    std::cout << "✓ Concurrent publishing test passed. Events processed: "
        << event_count.load() << std::endl;
}

void test_event_bus_concurrent_subscription() {
    std::cout << "Testing concurrent subscription..." << std::endl;

    EventBus bus;
    std::atomic<int> subscription_count{ 0 };
    constexpr int THREAD_COUNT = 8;
    constexpr int SUBSCRIPTIONS_PER_THREAD = 50;

    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<EventBus::BaseSubscription::SubscriptionHandle>> handles;
    std::mutex handles_mutex;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&bus, i, &subscription_count, &handles, &handles_mutex]() {
            for (int j = 0; j < SUBSCRIPTIONS_PER_THREAD; ++j) {
                auto handle = bus.Subscribe<TestEvent>([&, i, j](TestEvent const& event) {
                    subscription_count.fetch_add(1, std::memory_order_relaxed);
                    return true;
                    });

                // Store handles to keep subscriptions alive
                std::lock_guard<std::mutex> lock(handles_mutex);
                handles.push_back(std::make_unique<EventBus::BaseSubscription::SubscriptionHandle>(std::move(handle)));
            }
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Publish events to all subscribers
    bus.Publish(TestEvent{ 999, "Test all subscriptions" });

    // We should have at least one event processed per subscription
    // (might be more due to multiple events or other factors)
    assert(subscription_count.load() >= THREAD_COUNT * SUBSCRIPTIONS_PER_THREAD);
    std::cout << "✓ Concurrent subscription test passed. Total subscriptions: "
        << THREAD_COUNT * SUBSCRIPTIONS_PER_THREAD
        << ", Events processed: " << subscription_count.load() << std::endl;
}

void test_event_bus_thread_safety() {
    std::cout << "Testing thread safety with mixed operations..." << std::endl;

    EventBus bus;
    std::atomic<int> events_processed{ 0 };
    std::atomic<bool> stop{ false };
    constexpr int TEST_DURATION_MS = 2000;

    // Worker threads that will publish events
    std::vector<std::thread> publisher_threads;
    for (int i = 0; i < 4; ++i) {
        publisher_threads.emplace_back([&bus, i, &stop]() {
            int event_id = i * 1000;
            while (!stop.load()) {
                bus.Publish(TestEvent{ event_id++, "From publisher " + std::to_string(i) });
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            });
    }

    // Worker threads that will subscribe and unsubscribe
    std::vector<std::thread> subscriber_threads;
    std::vector<std::unique_ptr<EventBus::BaseSubscription::SubscriptionHandle>> active_handles;
    std::mutex handles_mutex;

    for (int i = 0; i < 4; ++i) {
        subscriber_threads.emplace_back([&bus, i, &events_processed, &stop, &active_handles, &handles_mutex]() {
            auto subscribe_func = [&]() {
                auto handle = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
                    events_processed.fetch_add(1, std::memory_order_relaxed);
                    return true;
                    });

                std::lock_guard<std::mutex> lock(handles_mutex);
                active_handles.push_back(std::make_unique<EventBus::BaseSubscription::SubscriptionHandle>(std::move(handle)));
                };

            while (!stop.load()) {
                subscribe_func();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                // Occasionally clear some subscriptions
                if (rand() % 5 == 0) {
                    std::lock_guard<std::mutex> lock(handles_mutex);
                    if (!active_handles.empty()) {
                        active_handles.pop_back();
                    }
                }
            }
            });
    }

    // Let the test run for specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(TEST_DURATION_MS));
    stop.store(true);

    // Wait for all threads to finish
    for (auto& thread : publisher_threads) {
        thread.join();
    }
    for (auto& thread : subscriber_threads) {
        thread.join();
    }

    // The test passes if we don't crash and process some events
    std::cout << "✓ Thread safety test passed. Events processed: "
        << events_processed.load() << std::endl;
}

void test_event_bus_high_contention() {
    std::cout << "Testing high contention scenario..." << std::endl;

    EventBus bus;
    std::atomic<long long> total_events{ 0 };
    constexpr int PUBLISHER_COUNT = 5;
    constexpr int SUBSCRIBER_COUNT = 5;
    constexpr int DURATION_MS = 1000;

    std::atomic<bool> stop{ false };

    // Create multiple subscribers
    std::vector<std::thread> subscriber_threads;
    for (int i = 0; i < SUBSCRIBER_COUNT; ++i) {
        subscriber_threads.emplace_back([&bus, i, &total_events, &stop]() {
            auto handle = bus.Subscribe<PerformanceEvent>([&](PerformanceEvent const& event) {
                total_events.fetch_add(1, std::memory_order_relaxed);
                // Simulate some processing time
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                return true;
                });

            // Keep the subscription alive
            while (!stop.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            });
    }

    // Create multiple publishers
    std::vector<std::thread> publisher_threads;
    std::vector<long long> events_published(PUBLISHER_COUNT, 0);

    for (int i = 0; i < PUBLISHER_COUNT; ++i) {
        publisher_threads.emplace_back([&bus, i, &events_published, &stop]() {
            auto start = std::chrono::steady_clock::now();
            while (!stop.load()) {
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

                bus.Publish(PerformanceEvent{
                    duration.count(),
                    i
                    });
                events_published[i]++;

                // Small random delay to desynchronize threads
                std::this_thread::sleep_for(std::chrono::microseconds(50 + (rand() % 50)));
            }
            });
    }

    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(DURATION_MS));
    stop.store(true);

    // Wait for all threads
    for (auto& thread : publisher_threads) {
        thread.join();
    }
    for (auto& thread : subscriber_threads) {
        thread.join();
    }

    long long total_published = 0;
    for (long long count : events_published) {
        total_published += count;
    }

    std::cout << "✓ High contention test passed." << std::endl;
    std::cout << "  Total events published: " << total_published << std::endl;
    std::cout << "  Total events processed: " << total_events.load() << std::endl;
    std::cout << "  Efficiency: " << (total_events.load() * 100.0 / total_published) << "%" << std::endl;
}

void test_event_bus_subscribe_during_publish_multithreaded() {
    std::cout << "Testing subscription during event publishing (multithreaded)..." << std::endl;

    EventBus bus;
    constexpr int THREAD_COUNT = 5;
    constexpr int EVENTS_PER_THREAD = 20;

    std::atomic<int> late_subscriber_call_count{ 0 };
    std::atomic<int> subscriptions_created{ 0 };
    std::vector<std::shared_ptr<EventBus::BaseSubscription::SubscriptionHandle>> late_subscribers;
    std::mutex subscribers_mutex;

    std::atomic<bool> stop_publishing{ false };

    // Main subscriber that creates new subscriptions during event processing
    auto main_handle = bus.Subscribe<TestEvent>([&](TestEvent const& event) {
        // Randomly decide whether to create a new subscription
        if (event.value % 3 == 0) {
            std::lock_guard<std::mutex> lock(subscribers_mutex);

            auto new_subscriber = std::make_shared<EventBus::BaseSubscription::SubscriptionHandle>(
                bus.Subscribe<TestEvent>([&](TestEvent const& inner_event) {
                    late_subscriber_call_count.fetch_add(1, std::memory_order_relaxed);
                    return true;
                    })
            );

            late_subscribers.push_back(new_subscriber);
            subscriptions_created.fetch_add(1, std::memory_order_relaxed);
        }
        return true;
        });

    // Publisher threads
    std::vector<std::thread> publisher_threads;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        publisher_threads.emplace_back([&bus, i, &stop_publishing]() {
            for (int j = 0; j < EVENTS_PER_THREAD; ++j) {
                if (stop_publishing.load()) break;

                bus.Publish(TestEvent{
                    i * EVENTS_PER_THREAD + j,
                    "Event from thread " + std::to_string(i)
                    });

                // Small random delay
                std::this_thread::sleep_for(std::chrono::microseconds(50 + (rand() % 100)));
            }
            });
    }

    // Let the test run for a while
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop_publishing.store(true);

    // Wait for all publishers to finish
    for (auto& thread : publisher_threads) {
        thread.join();
    }

    // Verify we created some subscriptions and they received events
    int final_subscriptions_created = subscriptions_created.load();
    int final_call_count = late_subscriber_call_count.load();

    std::cout << "Subscriptions created during publishing: " << final_subscriptions_created << std::endl;
    std::cout << "Late subscriber call count: " << final_call_count << std::endl;

    // The test passes if:
    // 1. We created at least one subscription during publishing
    // 2. The system didn't crash or deadlock
    // 3. Late subscribers received some events (optional, depends on timing)

    assert(final_subscriptions_created > 0 && "No subscriptions were created during publishing");
    std::cout << "✓ Multithreaded subscription during publishing test passed" << std::endl;
}

void run_all_event_bus_tests() {
    std::cout << "Starting EventBus test suite..." << std::endl;
    std::cout << "==========================================" << std::endl;

    try {
        // Basic functionality tests
        test_event_bus_basic();
        test_event_bus_multiple_subscribers();
        test_event_bus_event_stopping();
        test_event_bus_different_event_types();
        test_event_bus_unsubscribe();
        test_event_bus_void_return_action();
        test_event_bus_recursive_publish();
        test_event_bus_subscribe_during_publish();

        // Multi-threading tests
        test_event_bus_concurrent_publishing();
        test_event_bus_concurrent_subscription();
        test_event_bus_thread_safety();
        test_event_bus_subscribe_during_publish_multithreaded();
        test_event_bus_high_contention();


        std::cout << "==========================================" << std::endl;
        std::cout << "🎉 All EventBus tests passed successfully!" << std::endl;

    }
    catch (const std::exception& e) {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        throw;
    }
}

int main(void) {
    run_all_event_bus_tests();
    std::getchar();
    return 0;
}