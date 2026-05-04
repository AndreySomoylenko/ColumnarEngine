#include <cstring>
#include <cstdlib>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "data_structures/ByteVector.h"

TEST(ByteVectorTest, StartsEmptyAndAccumulatesRawBytes) {
    ByteVector bytes;
    const std::string first = "abc";
    const std::string second = "defg";

    bytes.Push(first.data(), first.size());
    bytes.Push(second.data(), second.size());

    EXPECT_EQ(bytes.Size(), 2U);
    EXPECT_EQ(bytes.SizeInBytes(), 7U);
    EXPECT_EQ(std::string(bytes.Data(), bytes.SizeInBytes()), "abcdefg");
}

TEST(ByteVectorTest, GrowsBeyondReservedCapacity) {
    ByteVector bytes;
    bytes.Reserve(2);

    const std::string payload = "payload";
    bytes.Push(payload.data(), payload.size());

    EXPECT_EQ(bytes.Size(), 1U);
    EXPECT_EQ(bytes.SizeInBytes(), payload.size());
    EXPECT_EQ(std::string(bytes.Data(), bytes.SizeInBytes()), payload);
}

TEST(ByteVectorTest, ClearKeepsStorageReusable) {
    ByteVector bytes;
    const std::string old_value = "old";
    const std::string new_value = "new";

    bytes.Push(old_value.data(), old_value.size());
    bytes.Clear();
    bytes.Push(new_value.data(), new_value.size());

    EXPECT_EQ(bytes.Size(), 1U);
    EXPECT_EQ(bytes.SizeInBytes(), new_value.size());
    EXPECT_EQ(std::string(bytes.Data(), bytes.SizeInBytes()), new_value);
}

TEST(ByteVectorTest, MoveTransfersBufferAndResetsSource) {
    ByteVector source;
    const std::string payload = "abc";
    source.Push(payload.data(), payload.size());

    ByteVector moved(std::move(source));

    EXPECT_EQ(moved.Size(), 1U);
    EXPECT_EQ(moved.SizeInBytes(), payload.size());
    EXPECT_EQ(std::string(moved.Data(), moved.SizeInBytes()), payload);
    EXPECT_EQ(source.Size(), 0U);
    EXPECT_EQ(source.SizeInBytes(), 0U);
    EXPECT_EQ(source.Data(), nullptr);
}

TEST(ByteVectorTest, FromBufferKeepsElementCountSeparateFromByteCount) {
    char *buffer = static_cast<char *>(malloc(5));
    std::memcpy(buffer, "abcde", 5);

    ByteVector bytes(2, 5, buffer);

    EXPECT_EQ(bytes.Size(), 2U);
    EXPECT_EQ(bytes.SizeInBytes(), 5U);
    EXPECT_EQ(std::string(bytes.Data(), bytes.SizeInBytes()), "abcde");
}
