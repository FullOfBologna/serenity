//FuncttionTable.h
#pragma once

#include <AK/HashMap.h>

namespace Py {

template <typename KeyType, typename ValueType>
class IdentifierMap{
public:
    IdentifierMap() = default;
    ~IdentifierMap() = default;

    void AddIdentifier(KeyType key, ValueType value);

    Value& getHashValue(Key key) const;
    
private:
    AK::HashMap<KeyType, ValueType> m_hashMap;
}

}