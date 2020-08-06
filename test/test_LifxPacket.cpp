#include <Arduino.h>
#include <unity.h>
#include "LifxPacket.hpp"

void check_size_LifxStructSize() {
    TEST_ASSERT_EQUAL(8, sizeof(LifxFrame));
    TEST_ASSERT_EQUAL(16, sizeof(LifxFrameAddress));
    TEST_ASSERT_EQUAL(12, sizeof(LifxProtocolHeader));
    TEST_ASSERT_EQUAL(36, sizeof(LifxPacket));
}

void setup() {
    UNITY_BEGIN();

    RUN_TEST(check_size_LifxStructSize);
}

void loop() {
    UNITY_END();
}
