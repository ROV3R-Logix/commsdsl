#pragma once

#include "Protocol.h"
#include "IntField.h"

namespace commsdsl
{

class EnumFieldImpl;
class COMMSDSL_API EnumField : public Field
{
    using Base = Field;
public:

    struct ValueInfo
    {
        std::intmax_t m_value = 0;
        unsigned m_sinceVersion = 0;
        unsigned m_deprecatedSince = Protocol::notYetDeprecated();
        std::string m_description;
        std::string m_displayName;
    };

    using Type = IntField::Type;
    using Values = std::map<std::string, ValueInfo>;
    using RevValues = std::multimap<std::intmax_t, std::string>;

    explicit EnumField(const EnumFieldImpl* impl);
    explicit EnumField(Field field);

    Type type() const;
    Endian endian() const;
    std::intmax_t defaultValue() const;
    const Values& values() const;
    const RevValues& revValues() const;
    bool isNonUniqueAllowed() const;
    bool isUnique() const;
    bool validCheckVersion() const;
    bool hexAssign() const;
};

inline
bool operator==(const EnumField::ValueInfo& i1, const EnumField::ValueInfo& i2)
{
    return (i1.m_value == i2.m_value) &&
           (i1.m_sinceVersion == i2.m_sinceVersion) &&
           (i1.m_deprecatedSince == i2.m_deprecatedSince);
}

} // namespace commsdsl
