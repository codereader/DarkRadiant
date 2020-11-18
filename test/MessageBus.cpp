#include "RadiantTest.h"

#include <future>
#include "imessagebus.h"

namespace test
{

using MessageBusTest = RadiantTest;

class CustomMessage1 : 
    public radiant::IMessage
{
public:
    static const std::size_t Id = radiant::IMessage::Type::UserDefinedMessagesGoHigherThanThis + 1000;

    std::size_t getId() const override
    {
        return Id;
    }
};

class CustomMessage2 :
    public radiant::IMessage
{
public:
    static const std::size_t Id = radiant::IMessage::Type::UserDefinedMessagesGoHigherThanThis + 1001;
    
    std::size_t getId() const override
    {
        return Id;
    }
};

TEST_F(MessageBusTest, Registration)
{
    auto counter1 = 0;
    auto& messageBus = GlobalRadiantCore().getMessageBus();

    auto listenerId1 = messageBus.addListener(CustomMessage1::Id, [&](radiant::IMessage&) { ++counter1; });

    CustomMessage1 msg1;
    messageBus.sendMessage(msg1);

    // Counter should have been increased
    EXPECT_EQ(counter1, 1);

    messageBus.removeListener(listenerId1);

    // Send another message, counter should not increase 
    messageBus.sendMessage(msg1);
    
    EXPECT_EQ(counter1, 1);
}

TEST_F(MessageBusTest, ChannelHandling)
{
    auto counter1 = 0;
    auto counter2 = 0;
    auto& messageBus = GlobalRadiantCore().getMessageBus();

    auto listenerId1 = messageBus.addListener(CustomMessage1::Id, [&](radiant::IMessage&) { ++counter1; });
    auto listenerId2 = messageBus.addListener(CustomMessage2::Id, [&](radiant::IMessage&) { ++counter2; });

    EXPECT_NE(listenerId1, listenerId2);

    CustomMessage1 msg1;
    messageBus.sendMessage(msg1);

    // Only one counter should have been increased
    EXPECT_EQ(counter1, 1);
    EXPECT_EQ(counter2, 0);
}

TEST_F(MessageBusTest, MultipleListenersOnChannel)
{
    auto counter1 = 0;
    auto& messageBus = GlobalRadiantCore().getMessageBus();

    auto listenerId1 = messageBus.addListener(CustomMessage1::Id, [&](radiant::IMessage&) { ++counter1; });
    auto listenerId2 = messageBus.addListener(CustomMessage1::Id, [&](radiant::IMessage&) { ++counter1; });

    CustomMessage1 msg1;
    messageBus.sendMessage(msg1);

    // Counter should have been increased twice
    EXPECT_EQ(counter1, 2);

    messageBus.removeListener(listenerId1);

    // Send another message, counter should increase only once now
    messageBus.sendMessage(msg1);

    EXPECT_EQ(counter1, 3);
}

TEST_F(MessageBusTest, DeregistrationDuringCallback)
{
    auto counter1 = 0;
    auto& messageBus = GlobalRadiantCore().getMessageBus();

    // Two listeners, unsubscribing during callback
    std::size_t listenerId1 = 0;
    std::size_t listenerId2 = 0;

    listenerId1 = messageBus.addListener(CustomMessage1::Id, [&](radiant::IMessage&)
    {
        ++counter1;
        messageBus.removeListener(listenerId1);
    });

    listenerId2 = messageBus.addListener(CustomMessage1::Id, [&](radiant::IMessage&)
    {
        ++counter1;
        messageBus.removeListener(listenerId2);
    });

    CustomMessage1 msg1;
    messageBus.sendMessage(msg1);

    // Counter should have been increased twice
    EXPECT_EQ(counter1, 2);

    // Send another message, counter should not increase anymore
    messageBus.sendMessage(msg1);

    EXPECT_EQ(counter1, 2);
}

TEST_F(MessageBusTest, MultipleThreadsCanSendMessages)
{
    std::size_t counter = 0;
    auto& messageBus = GlobalRadiantCore().getMessageBus();

    // Add a handler that launches another thread which in turn is sending messages
    auto listenerId1 = messageBus.addListener(CustomMessage1::Id, [&](radiant::IMessage&) 
    {
        ++counter;

        // Launch a separate thread which is sending messages
        std::thread innerThread([&]()
        {
            CustomMessage2 msg2;
            messageBus.sendMessage(msg2);
        });

        // If the sendMessage(CustomMessage2) deadlocks, this will never return
        innerThread.join();

        ++counter; // finish => counter == 2
    });

    auto task = std::async(std::launch::async, [&]()
    {
        CustomMessage1 msg1;
        messageBus.sendMessage(msg1);
    });

    auto result = task.wait_for(std::chrono::seconds(2));

    EXPECT_EQ(counter, 2);
    EXPECT_EQ(result, std::future_status::ready) << "Inner thread doesn't respond, possibly dead-locked";

    if (result == std::future_status::timeout)
    {
        // In case of a deadlock the destructor of the above task
        // would wait forever, abort the whole test application
        std::terminate();
    }
}

}
