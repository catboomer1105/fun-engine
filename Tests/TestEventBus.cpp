#include <gtest/gtest.h>
#include <Core/Event/EventBus.h>

using namespace fun;

TEST(Event, SetAndGetType) {
    Event e("Damage");
    EXPECT_EQ(e.GetType(), "Damage");
}

TEST(Event, SetAndGetInt) {
    Event e("Damage");
    e.Set("amount", 25);
    auto result = e.Get<int>("amount");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 25);
}

TEST(Event, SetAndGetString) {
    Event e("Event");
    e.Set("name", std::string("Player"));
    auto result = e.Get<std::string>("name");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Player");
}

TEST(Event, GetMissingKeyReturnsNullopt) {
    Event e("Test");
    auto result = e.Get<int>("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST(Event, WrongTypeReturnsNullopt) {
    Event e("Test");
    e.Set("value", 42);
    auto result = e.Get<std::string>("value");  // int -> string mismatch
    EXPECT_FALSE(result.has_value());
}

TEST(Event, HasKey) {
    Event e("Test");
    EXPECT_FALSE(e.Has("key"));
    e.Set("key", 1);
    EXPECT_TRUE(e.Has("key"));
}

TEST(EventBus, SubscribeAndEmit) {
    EventBus bus;
    bool received = false;
    int receivedAmount = 0;

    bus.Subscribe("Damage", [&](const Event& e) {
        received = true;
        receivedAmount = e.Get<int>("amount").value_or(0);
    });

    Event e("Damage");
    e.Set("amount", 25);
    bus.Emit(e);

    EXPECT_TRUE(received);
    EXPECT_EQ(receivedAmount, 25);
}

TEST(EventBus, Unsubscribe) {
    EventBus bus;
    int callCount = 0;

    auto id = bus.Subscribe("Test", [&](const Event&) {
        callCount++;
    });

    Event e("Test");
    bus.Emit(e);
    EXPECT_EQ(callCount, 1);

    bus.Unsubscribe(id);
    bus.Emit(e);
    EXPECT_EQ(callCount, 1);  // 不应再被调用
}

TEST(EventBus, MultipleSubscribers) {
    EventBus bus;
    int count1 = 0, count2 = 0;

    bus.Subscribe("Test", [&](const Event&) { count1++; });
    bus.Subscribe("Test", [&](const Event&) { count2++; });

    Event e("Test");
    bus.Emit(e);

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}

TEST(EventBus, EmitWithNoSubscribers) {
    EventBus bus;
    Event e("NoSubscriber");
    EXPECT_NO_THROW(bus.Emit(e));
}
