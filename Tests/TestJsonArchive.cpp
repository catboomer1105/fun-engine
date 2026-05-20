#include <gtest/gtest.h>
#include <Core/Serialization/JsonArchive.h>
#include <cmath>
#include <cstdio>

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

TEST(JsonArchive, SaveToFileAndLoadFromFile) {
    const std::string path = "test_json_archive_temp.json";

    // 写
    JsonArchive writer;
    int intValue = 123;
    std::string strValue = "file test";
    float floatValue = 4.5f;
    writer("int", intValue);
    writer("string", strValue);
    writer("float", floatValue);

    // 保存到文件
    ASSERT_TRUE(writer.SaveToFile(path));

    // 从文件加载
    JsonArchive reader = JsonArchive::LoadFromFile(path);
    EXPECT_TRUE(reader.IsReading());

    int readInt = 0;
    std::string readStr;
    float readFloat = 0;
    reader("int", readInt);
    reader("string", readStr);
    reader("float", readFloat);

    EXPECT_EQ(readInt, 123);
    EXPECT_EQ(readStr, "file test");
    EXPECT_NEAR(readFloat, 4.5f, 0.001f);

    // 清理
    std::remove(path.c_str());
}

TEST(JsonArchive, LoadFromFileNonExistentReturnsEmpty) {
    JsonArchive reader = JsonArchive::LoadFromFile("__nonexistent_file__.json");
    EXPECT_TRUE(reader.IsReading());
    // 读取不应崩溃
    int value = 42;
    reader("anything", value);
    EXPECT_EQ(value, 42);
}

TEST(JsonArchive, SaveToFileInvalidPathReturnsFalse) {
    JsonArchive writer;
    int testVal = 1;
    writer("test", testVal);
    // 使用无效路径（Windows 上含非法字符或不存在目录）
    EXPECT_FALSE(writer.SaveToFile("Z:/__nonexistent_dir__/__test__.json"));
}

TEST(JsonArchive, RoundTripViaFile) {
    const std::string path = "test_json_roundtrip.json";

    // 嵌套结构 + 数组
    {
        JsonArchive writer;
        std::string sceneName = "Arena";
        writer("name", sceneName);
        writer.Push("transform");
        float pos[3] = {1.0f, 2.0f, 3.0f};
        writer.Vec3Serialize("position", pos);
        writer.Pop();
        ASSERT_TRUE(writer.SaveToFile(path));
    }

    // 读回
    {
        JsonArchive reader = JsonArchive::LoadFromFile(path);
        std::string name;
        reader("name", name);
        EXPECT_EQ(name, "Arena");

        reader.Push("transform");
        float pos[3] = {};
        reader.Vec3Serialize("position", pos);
        reader.Pop();

        EXPECT_NEAR(pos[0], 1.0f, 0.001f);
        EXPECT_NEAR(pos[1], 2.0f, 0.001f);
        EXPECT_NEAR(pos[2], 3.0f, 0.001f);
    }

    std::remove(path.c_str());
}
