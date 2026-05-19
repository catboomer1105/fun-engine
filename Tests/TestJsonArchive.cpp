#include <gtest/gtest.h>
#include <Core/Serialization/JsonArchive.h>
#include <cmath>

using namespace fun;

TEST(JsonArchive, WriteAndReadBasicTypes) {
    // 写
    JsonArchive writer;
    int intValue = 42;
    float floatValue = 3.14f;
    double doubleValue = 2.718;
    bool boolValue = true;
    std::string strValue = "hello";
    writer("int", intValue);
    writer("float", floatValue);
    writer("double", doubleValue);
    writer("bool", boolValue);
    writer("string", strValue);

    std::string jsonStr = writer.ToString();
    EXPECT_FALSE(jsonStr.empty());

    // 读
    JsonArchive reader(jsonStr);
    int readInt = 0;
    float readFloat = 0;
    double readDouble = 0;
    bool readBool = false;
    std::string readStr;
    reader("int", readInt);
    reader("float", readFloat);
    reader("double", readDouble);
    reader("bool", readBool);
    reader("string", readStr);

    EXPECT_EQ(readInt, 42);
    EXPECT_NEAR(readFloat, 3.14f, 0.001f);
    EXPECT_NEAR(readDouble, 2.718, 0.001);
    EXPECT_TRUE(readBool);
    EXPECT_EQ(readStr, "hello");
}

TEST(JsonArchive, NestedObject) {
    JsonArchive writer;
    writer.Push("nested");
    float value = 99.5f;
    writer("value", value);
    writer.Pop();

    std::string jsonStr = writer.ToString();

    JsonArchive reader(jsonStr);
    reader.Push("nested");
    float readValue = 0;
    reader("value", readValue);
    reader.Pop();

    EXPECT_NEAR(readValue, 99.5f, 0.001f);
}

TEST(JsonArchive, Vec3Serialize) {
    JsonArchive writer;
    float data[3] = {1.0f, 2.0f, 3.0f};
    writer.Vec3Serialize("position", data);

    std::string jsonStr = writer.ToString();

    JsonArchive reader(jsonStr);
    float readData[3] = {};
    reader.Vec3Serialize("position", readData);

    EXPECT_NEAR(readData[0], 1.0f, 0.001f);
    EXPECT_NEAR(readData[1], 2.0f, 0.001f);
    EXPECT_NEAR(readData[2], 3.0f, 0.001f);
}

TEST(JsonArchive, QuatSerialize) {
    JsonArchive writer;
    float data[4] = {0.0f, 0.707f, 0.0f, 0.707f};
    writer.QuatSerialize("rotation", data);

    std::string jsonStr = writer.ToString();

    JsonArchive reader(jsonStr);
    float readData[4] = {};
    reader.QuatSerialize("rotation", readData);

    EXPECT_NEAR(readData[0], 0.0f, 0.001f);
    EXPECT_NEAR(readData[1], 0.707f, 0.001f);
    EXPECT_NEAR(readData[2], 0.0f, 0.001f);
    EXPECT_NEAR(readData[3], 0.707f, 0.001f);
}

TEST(JsonArchive, IsReadingIsWriting) {
    JsonArchive writer;
    EXPECT_TRUE(writer.IsWriting());
    EXPECT_FALSE(writer.IsReading());

    JsonArchive reader("{}");
    EXPECT_TRUE(reader.IsReading());
    EXPECT_FALSE(reader.IsWriting());
}

TEST(JsonArchive, ReadMissingKeyDoesNotCrash) {
    JsonArchive reader("{}");
    int value = 999;
    reader("nonexistent", value);
    EXPECT_EQ(value, 999);  // 应保持原值
}
