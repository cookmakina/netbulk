#include <gtest/gtest.h>
#include <iostream>
#include "utils.h"
#include "command_buffer.h"


TEST(CommandBuffer, Simple)
{
    std::ostringstream result;

    CommandBuffer buf(3, [&result](const Bulk& bulk) { result << bulk << "\n"; });
    for (int i = 0; i < 3; i++)
        buf.pushCommand("1");

    ASSERT_EQ(buf.getLinesCount(), 3);
    ASSERT_EQ(buf.getCommandsCount(), 3);
    ASSERT_EQ(buf.getBulksCount(), 1);

    ASSERT_EQ(result.str(), "bulk: 1, 1, 1\n");
}

TEST(CommandBuffer, Single)
{
    std::ostringstream result;

    CommandBuffer buf(1, [&result](const Bulk& bulk) { result << bulk << "\n"; });
    for (int i = 0; i < 3; i++) {
        buf.pushCommand("1");
    }

    ASSERT_EQ(buf.getLinesCount(), 3);
    ASSERT_EQ(buf.getCommandsCount(), 3);
    ASSERT_EQ(buf.getBulksCount(), 3);

    ASSERT_EQ(result.str(), "bulk: 1\nbulk: 1\nbulk: 1\n");
}

TEST(CommandBuffer, Many)
{
    std::ostringstream result;

    CommandBuffer buf(2, [&result](const Bulk& bulk) { result << bulk << "\n"; });
    for (int i = 0; i < 4; i++) {
        buf.pushCommand("o");
    }

    ASSERT_EQ(buf.getLinesCount(), 4);
    ASSERT_EQ(buf.getCommandsCount(), 4);
    ASSERT_EQ(buf.getBulksCount(), 2);

    ASSERT_EQ(result.str(), "bulk: o, o\nbulk: o, o\n");
}

TEST(CommandBuffer, Flush)
{
    std::ostringstream result;
    {
        CommandBuffer buf(2, [&result](const Bulk& bulk) { result << bulk << "\n"; });
        for (int i = 0; i < 3; i++) {
            buf.pushCommand("k");
        }
        buf.flush();

        ASSERT_EQ(buf.getLinesCount(), 3);
        ASSERT_EQ(buf.getCommandsCount(), 3);
        ASSERT_EQ(buf.getBulksCount(), 2);
    }
    ASSERT_EQ(result.str(), "bulk: k, k\nbulk: k\n");
}

TEST(CommandBuffer, Group)
{
    std::ostringstream result;
    {
        CommandBuffer buf(2, [&result](const Bulk& bulk) { result << bulk << "\n"; });
        buf.pushCommand("0"); // a group breaks previously unfinished bulk (thus sending it to output)
        buf.pushGroup();
            buf.pushCommand("1");
            buf.pushCommand("2");
            buf.pushCommand("3");
        buf.popGroup();

        ASSERT_EQ(buf.getLinesCount(), 6);
        ASSERT_EQ(buf.getCommandsCount(), 4);
        ASSERT_EQ(buf.getBulksCount(), 2);
    }
    ASSERT_EQ(result.str(), "bulk: 0\nbulk: 1, 2, 3\n");
}

TEST(CommandBuffer, Other)
{
    std::ostringstream result;
    {
        CommandBuffer buf(1, [&result](const Bulk& bulk) { result << bulk << "\n"; });
        buf.pushGroup();
        buf.pushCommand("cmd1");
        buf.pushCommand("cmd2");
        buf.pushGroup();
        buf.pushCommand("cmd3");
        buf.pushCommand("cmd4");
        buf.popGroup();
        buf.pushCommand("cmd5");
        buf.pushCommand("cmd6");
        buf.popGroup();

        ASSERT_EQ(buf.getLinesCount(), 10);
        ASSERT_EQ(buf.getCommandsCount(), 6);
        ASSERT_EQ(buf.getBulksCount(), 1);
    }
    ASSERT_EQ(result.str(), "bulk: cmd1, cmd2, cmd3, cmd4, cmd5, cmd6\n");
}

TEST(CommandBuffer, GroupNested)
{
    std::ostringstream result;
    {
        // nested {} doesn't do anything
        CommandBuffer buf(2, [&result](const Bulk& bulk) { result << bulk << "\n"; });
        buf.pushGroup();
            buf.pushCommand("1");
            buf.pushGroup();
                buf.pushCommand("2");
            buf.popGroup();
            buf.pushCommand("3");
        buf.popGroup();

        ASSERT_EQ(buf.getLinesCount(), 7);
        ASSERT_EQ(buf.getCommandsCount(), 3);
        ASSERT_EQ(buf.getBulksCount(), 1);
    }
    ASSERT_EQ(result.str(), "bulk: 1, 2, 3\n");
}

TEST(CommandBuffer, GroupTerminate)
{
    std::ostringstream result;
    {
        // unfinished group should be terminated if it hasn't been closed with }
        CommandBuffer buf(2, [&result](const Bulk& bulk) { result << bulk << "\n"; });
        buf.pushGroup();
        buf.pushCommand("1");
        buf.pushCommand("2");
        buf.pushCommand("3");

        ASSERT_EQ(buf.getLinesCount(), 4);
        ASSERT_EQ(buf.getCommandsCount(), 3);
        ASSERT_EQ(buf.getBulksCount(), 0);
    }
    ASSERT_EQ(result.str(), "");
}

///////////////////////////////////
TEST(CommandBuffer, ParseString)
{
    std::ostringstream result;
    {
        // unfinished group should be terminated if it hasn't been closed with }
        CommandBuffer buf(2, [&result](const Bulk& bulk) { result << bulk << "\n"; });
        buf.receiveText("{");
        buf.receiveText("\ncmd1");
        buf.receiveText("\n}\n");

        ASSERT_EQ(buf.getLinesCount(), 3);
        ASSERT_EQ(buf.getCommandsCount(), 1);
        ASSERT_EQ(buf.getBulksCount(), 1);
    }
    ASSERT_EQ(result.str(), "bulk: cmd1\n");
}

TEST(CommandBuffer, ParseString2)
{
    std::ostringstream result;
    {
        // unfinished group should be terminated if it hasn't been closed with }
        CommandBuffer buf(3, [&result](const Bulk& bulk) { result << bulk << "\n"; });
        buf.receiveText("cm");
        buf.receiveText("d1\n");
        buf.receiveText("cmd2\n");
        buf.receiveText("cmd3");
        buf.receiveText("\n");

        ASSERT_EQ(buf.getLinesCount(), 3);
        ASSERT_EQ(buf.getCommandsCount(), 3);
        ASSERT_EQ(buf.getBulksCount(), 1);
    }
    ASSERT_EQ(result.str(), "bulk: cmd1, cmd2, cmd3\n");
}

TEST(CommandBuffer, ParseString3)
{
    std::ostringstream result;
    {
        // unfinished group should be terminated if it hasn't been closed with }
        CommandBuffer buf(1, [&result](const Bulk& bulk) { result << bulk << "\n"; });
        buf.receiveText("{\ncmd1\ncmd2");
        buf.receiveText("\n{\ncmd3\n");
        buf.receiveText("cmd4\n}\n");
        buf.receiveText("cmd5\ncm");
        buf.receiveText("d6\n}\n");

        ASSERT_EQ(buf.getLinesCount(), 10);
        ASSERT_EQ(buf.getCommandsCount(), 6);
        ASSERT_EQ(buf.getBulksCount(), 1);
    }
    ASSERT_EQ(result.str(), "bulk: cmd1, cmd2, cmd3, cmd4, cmd5, cmd6\n");
}
///////////////////////////////////
TEST(Utils, StripSpaces)
{
    std::string str;

    str = "abc";
    stripSpaces(str);
    ASSERT_EQ(str, "abc");

    str = " a";
    stripSpaces(str);
    ASSERT_EQ(str, "a");

    str = "b ";
    stripSpaces(str);
    ASSERT_EQ(str, "b");

    str = " c ";
    stripSpaces(str);
    ASSERT_EQ(str, "c");

    str = " ";
    stripSpaces(str);
    ASSERT_EQ(str, "");

    str = "    ";
    stripSpaces(str);
    ASSERT_EQ(str, "");

    str = "";
    stripSpaces(str);
    ASSERT_EQ(str, "");
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}