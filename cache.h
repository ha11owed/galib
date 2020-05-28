#pragma once

#include "intrusive_containers.h"

namespace galib {

template <typename TCacheKey, typename TCacheValue, unsigned int TMaxLevel> class Cache {
  public:
    struct KeyValue {
        TCacheValue data;

        // Used by the intrusive dictionary
        TCacheKey _key;
        size_t _lastMemSize;

        Link<KeyValue> _listLink;
        Link<KeyValue> _dictLink;
    };

    using CacheValueList = List<KeyValue, &KeyValue::_listLink>;
    using CacheValueDict = Dictionary<KeyValue, TCacheKey, &KeyValue::_key, &KeyValue::_dictLink>;

    struct CacheLevel {
        CacheValueList _list;
        CacheValueDict _dict;

        unsigned int _count = 0;
        unsigned int _maxCount = 1024 * 16;

        size_t _memUsage = 0;
        size_t _maxMemUsage = 1024 * 1024 * 32;

        bool isOverCapacity() const { return (_count > _maxCount) || (_memUsage > _maxMemUsage); }
    };

  public:
    virtual ~Cache() {}

    void configureLevel(int level, unsigned int maxCount, size_t maxMemUsage) {
        if (level >= 0 && level < TMaxLevel) {
            _levels[level]._maxCount = maxCount;
            _levels[level]._maxMemUsage = maxMemUsage;
        }
    }

  protected:
    virtual KeyValue *newCacheValue(const TCacheKey &, int) { return new KeyValue(); }

    virtual void onCacheLevelChanged(KeyValue *, int, int) {}

    virtual size_t estimateMemSize(KeyValue *) { return sizeof(KeyValue); }

  public:
    TCacheValue find(const TCacheKey &key) const {
        KeyValue *value = findKeyValue(key);
        if (value != nullptr) {
            return value->data;
        }
        return TCacheValue();
    }

    TCacheValue *findPtr(const TCacheKey &key) const {
        KeyValue *value = findKeyValue(key);
        if (value != nullptr) {
            return &value->data;
        }
        return nullptr;
    }

    TCacheValue get(const TCacheKey &key, int levelIndex = -1) {
        KeyValue *kv = getKeyValue(key, levelIndex);
        if (kv != nullptr) {
            return kv->data;
        }
        return TCacheValue();
    }

    TCacheValue *getPtr(const TCacheKey &key, int levelIndex = -1) {
        KeyValue *kv = getKeyValue(key, levelIndex);
        if (kv != nullptr) {
            return &(kv->data);
        }
        return nullptr;
    }

    KeyValue *getKeyValue(const TCacheKey &key, int levelIndex = -1) {
        // Find the value
        int oldLevelIndex = -1;
        KeyValue *value = findKeyValue(key, &oldLevelIndex);

        if (levelIndex < 0) {
            if (oldLevelIndex >= 0) {
                levelIndex = oldLevelIndex;
            } else {
                levelIndex = 0;
            }
        } else if (levelIndex >= TMaxLevel) {
            levelIndex = TMaxLevel - 1;
        }

        if (value == nullptr) {
            // The key does not exist
            value = newCacheValue(key, levelIndex);

            // Check that the file exists
            if (value == nullptr) {
                return nullptr;
            }

            value->_key = key;
            value->_lastMemSize = estimateMemSize(value);

            CacheLevel &lvl = _levels[levelIndex];
            lvl._list.insertHead(value);
            lvl._dict.put(value);
            lvl._count++;
            lvl._memUsage += value->_lastMemSize;

            ensureLevelLimits(levelIndex);
        } else {
            // The level of the object has maybe changed
            CacheLevel &lvl = _levels[oldLevelIndex];
            value->_listLink.unlink();

            if (oldLevelIndex == levelIndex) {
                // Move the item to the head of the list
                lvl._list.insertHead(value);
            } else {
                // Move the item to a new level
                value->_dictLink.unlink();
                lvl._count--;
                lvl._memUsage -= value->_lastMemSize;

                onCacheLevelChanged(value, oldLevelIndex, levelIndex);
                value->_lastMemSize = estimateMemSize(value);

                CacheLevel &newLvl = _levels[levelIndex];
                newLvl._list.insertHead(value);
                newLvl._dict.put(value);
                newLvl._count++;
                lvl._memUsage += value->_lastMemSize;

                ensureLevelLimits(levelIndex);
            }
        }

        return value;
    }

    void remove(const TCacheKey &key) {
        int levelIndex = -1;
        KeyValue *value = findKeyValue(key, &levelIndex);
        if (value != nullptr) {
            _levels[levelIndex]._count--;
            _levels[levelIndex]._memUsage -= value->_lastMemSize;
            delete value;
        }
    }

  private:
    KeyValue *findKeyValue(const TCacheKey &key, int *level = nullptr) const {
        for (int i = 0; i < TMaxLevel; i++) {
            KeyValue *value = _levels[i]._dict.get(key);
            if (value != nullptr) {
                if (level != nullptr) {
                    *level = i;
                }
                return value;
            }
        }
        return nullptr;
    }

    void ensureLevelLimits(int level) {
        for (int i = level; i < TMaxLevel - 1; i++) {
            CacheLevel &lvl = _levels[i];
            CacheLevel &nextLvl = _levels[i + 1];
            while (lvl.isOverCapacity() && lvl._count > 1) {
                // we need to move the last item to next
                KeyValue *last = lvl._list.tail();
                last->_listLink.unlink();
                last->_dictLink.unlink();
                lvl._count--;
                lvl._memUsage -= last->_lastMemSize;

                onCacheLevelChanged(last, i, i + 1);
                last->_lastMemSize = estimateMemSize(last);

                nextLvl._dict.put(last);
                nextLvl._list.insertHead(last);
                nextLvl._count++;
                nextLvl._memUsage += last->_lastMemSize;
            }
        }
    }

  private:
    CacheLevel _levels[TMaxLevel];
};

} // namespace galib
