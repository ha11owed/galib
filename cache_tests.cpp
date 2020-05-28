#include "cache.h"
#include "gtest/gtest.h"

using namespace galib;

struct StringCacheValue {
    StringCacheValue(const std::string &value_, int level_, int oldLevel_)
        : value(value_)
        , level(level_)
        , oldLevel(oldLevel_) {}
    StringCacheValue() {}

    std::string value;
    int level = -1;
    int oldLevel = -1;
};

bool operator==(const StringCacheValue &a, const StringCacheValue &b) {
    return (a.value == b.value) && (a.level == b.level) && (a.oldLevel == b.oldLevel);
}

class StringCache : public Cache<std::string, StringCacheValue, 2> {
  protected:
    KeyValue *newCacheValue(const std::string &key, int lvl) override {
        KeyValue *kv = new KeyValue;
        kv->data.value = key;
        kv->data.level = lvl;
        return kv;
    }

    void onCacheLevelChanged(KeyValue *kv, int oldLvl, int newLvl) override {
        kv->data.level = newLvl;
        kv->data.oldLevel = oldLvl;
    }
};

TEST(CacheTest, BasicGetRemove) {
    StringCache cache;
    // The level 0 cache can only hold 1 element.
    cache.configureLevel(0, 1, 99999);

    EXPECT_EQ(StringCacheValue("a", 0, -1), cache.get("a"));
    EXPECT_EQ(StringCacheValue("b", 0, -1), cache.get("b"));
    // b causes a to go from lvl0 to lvl1.
    EXPECT_EQ(StringCacheValue("a", 1, 0), cache.get("a"));

    cache.getPtr("a")->value = "aa";
    cache.getPtr("b")->value = "bb";

    EXPECT_EQ(StringCacheValue("aa", 1, 0), cache.get("a"));
    EXPECT_EQ(StringCacheValue("bb", 0, -1), cache.get("b"));

    cache.remove("a");

    EXPECT_EQ(StringCacheValue("a", 0, -1), cache.get("a"));
    EXPECT_EQ(StringCacheValue("bb", 1, 0), cache.get("b"));
}
