#include "FieldImpl.h"

#include <cassert>
#include <limits>
#include <algorithm>
#include <set>

#include "ProtocolImpl.h"
#include "IntFieldImpl.h"
#include "FloatFieldImpl.h"
#include "BitfieldFieldImpl.h"
#include "BundleFieldImpl.h"
#include "common.h"

namespace bbmp
{

FieldImpl::Ptr FieldImpl::create(
    const std::string& kind,
    ::xmlNodePtr node,
    ProtocolImpl& protocol)
{
    auto& map = createMap();

    auto iter = map.find(kind);
    if (iter == map.end()) {
        return Ptr();
    }

    return iter->second(node, protocol);
}

bool FieldImpl::parse()
{
    m_props = XmlWrap::parseNodeProps(m_node);

    if (!XmlWrap::parseChildrenAsProps(m_node, commonProps(), m_protocol.logger(), m_props)) {
        return false;
    }

    auto& extraPropsNames = extraPropsNamesImpl();
    do {
        if (extraPropsNames.empty()) {
            break;
        }

        if (!XmlWrap::parseChildrenAsProps(m_node, extraPropsNames, m_protocol.logger(), m_props)) {
            return false;
        }

    } while (false);

    auto& extraPossiblePropsNames = extraPossiblePropsNamesImpl();
    do {
        if (extraPossiblePropsNames.empty()) {
            break;
        }

        if (!XmlWrap::parseChildrenAsProps(m_node, extraPossiblePropsNames, m_protocol.logger(), m_props, false)) {
            return false;
        }

    } while (false);

    bool result =
        updateName() &&
        updateDisplayName() &&
        updateDescription() &&
        updateSinceVersion() &&
        updateDeprecated();

    if (!result) {
        return false;
    }

    if (!parseImpl()) {
        return false;
    }

    XmlWrap::NamesList expectedProps = commonProps();
    expectedProps.insert(expectedProps.end(), extraPropsNames.begin(), extraPropsNames.end());
    expectedProps.insert(expectedProps.end(), extraPossiblePropsNames.begin(), extraPossiblePropsNames.end());
    m_unknownAttrs = XmlWrap::getUnknownProps(m_node, expectedProps);

    auto& commonCh = commonChildren();
    auto& extraChildren = extraChildrenNamesImpl();
    XmlWrap::NamesList expectedChildren = commonProps();
    expectedChildren.insert(expectedChildren.end(), commonCh.begin(), commonCh.end());
    expectedChildren.insert(expectedChildren.end(), extraPropsNames.begin(), extraPropsNames.end());
    expectedChildren.insert(expectedChildren.end(), extraPossiblePropsNames.begin(), extraPossiblePropsNames.end());
    expectedChildren.insert(expectedChildren.end(), extraChildren.begin(), extraChildren.end());
    m_unknownChildren = XmlWrap::getUnknownChildrenContents(m_node, expectedChildren);
    return true;
}

const std::string& FieldImpl::name() const
{
    assert(m_name != nullptr);
    return *m_name;
}

const std::string& FieldImpl::displayName() const
{
    assert(m_displayName != nullptr);
    return *m_displayName;
}

const std::string& FieldImpl::description() const
{
    assert(m_description != nullptr);
    return *m_description;
}

XmlWrap::NamesList FieldImpl::supportedTypes()
{
    XmlWrap::NamesList result;
    auto& map = createMap();
    result.reserve(map.size());
    std::transform(
        map.begin(), map.end(), std::back_inserter(result),
        [](auto& elem)
        {
            return elem.first;
        });
    return result;
}

bool FieldImpl::validateMembersVersions(
    const Object& obj,
    const FieldImpl::FieldsList& fields,
    Logger& logger)
{
    if (fields.size() < 2U) {
        return true;
    }

    for (auto idx = 0U; idx < (fields.size() - 1); ++idx) {
        auto& thisMem = fields[idx];
        auto& nextMem = fields[idx + 1];

        assert(obj.getMinSinceVersion() <= thisMem->getMinSinceVersion());

        if (nextMem->getMinSinceVersion() < thisMem->getMaxSinceVersion()) {
            bbmp::logError(logger) << XmlWrap::logPrefix(nextMem->getNode()) <<
                "Version of the member \"" << nextMem->name() << "\" (" <<
                nextMem->getMinSinceVersion() << ") must't be less than previous "
                " member's one (" << thisMem->getMaxSinceVersion() << ").";
            return false;
        }

        assert(thisMem->getMaxSinceVersion() <= obj.getMaxSinceVersion());
    }
    return true;
}

bool FieldImpl::validateMembersVersions(const FieldImpl::FieldsList& fields)
{
    return validateMembersVersions(*this, fields, protocol().logger());
}

bool FieldImpl::validateMembersNames(
    const FieldImpl::FieldsList& fields,
    Logger& logger)
{
    std::set<std::string> usedNames;
    for (auto& f : fields) {
        if (usedNames.find(f->name()) != usedNames.end()) {
            bbmp::logError(logger) << XmlWrap::logPrefix(f->getNode()) <<
                "Member field with name \"" << f->name() << "\" has already been defined.";
            return false;
        }
        usedNames.insert(f->name());
    }
    return true;
}

bool FieldImpl::validateMembersNames(const FieldImpl::FieldsList& fields)
{
    return validateMembersNames(fields, protocol().logger());
}

FieldImpl::FieldImpl(::xmlNodePtr node, ProtocolImpl& protocol)
  : m_node(node),
    m_protocol(protocol),
    m_name(&common::emptyString()),
    m_displayName(&common::emptyString()),
    m_description(&common::emptyString())
{
}

FieldImpl::FieldImpl(const FieldImpl&) = default;

LogWrapper FieldImpl::logError() const
{
    return bbmp::logError(m_protocol.logger());
}

LogWrapper FieldImpl::logWarning() const
{
    return bbmp::logWarning(m_protocol.logger());
}

Object::ObjKind FieldImpl::objKindImpl() const
{
    return ObjKind::SimpleField;
}

const XmlWrap::NamesList& FieldImpl::extraPropsNamesImpl() const
{
    return XmlWrap::emptyNamesList();
}

const XmlWrap::NamesList&FieldImpl::extraPossiblePropsNamesImpl() const
{
    return XmlWrap::emptyNamesList();
}

const XmlWrap::NamesList&FieldImpl::extraChildrenNamesImpl() const
{
    static const XmlWrap::NamesList Names;
    return Names;
}

bool FieldImpl::parseImpl()
{
    return true;
}

bool FieldImpl::validateImpl()
{
    return true;
}

std::size_t FieldImpl::bitLengthImpl() const
{
    return length() * std::numeric_limits<std::uint8_t>::digits;
}

bool FieldImpl::validateSinglePropInstance(const std::string& str, bool mustHave)
{
    return XmlWrap::validateSinglePropInstance(m_node, m_props, str, protocol().logger(), mustHave);
}

bool FieldImpl::validateAndUpdateStringPropValue(
    const std::string& str,
    const std::string*& valuePtr,
    bool mustHave)
{
    if (!validateSinglePropInstance(str, mustHave)) {
        return false;
    }

    auto iter = m_props.find(str);
    if (iter != m_props.end()) {
        valuePtr = &iter->second;
    }

    assert(iter != m_props.end() || (!mustHave));
    return true;
}

void FieldImpl::reportUnexpectedPropertyValue(const std::string& propName, const std::string& propValue)
{
    logError() << XmlWrap::logPrefix(m_node) <<
                  "Property \"" << propName << "\" of element \"" << name() <<
                  "\" has unexpected value (" << propValue << ").";
}

const XmlWrap::NamesList& FieldImpl::commonProps()
{
    static const XmlWrap::NamesList CommonNames = {
        common::nameStr(),
        common::displayNameStr(),
        common::descriptionStr(),
        common::sinceVersionStr(),
        common::deprecatedStr()
    };

    return CommonNames;
}

const XmlWrap::NamesList& FieldImpl::commonChildren()
{
    static const XmlWrap::NamesList CommonChildren = {
        common::metaStr()
    };

    return CommonChildren;
}

bool FieldImpl::updateName()
{
    return validateAndUpdateStringPropValue(common::nameStr(), m_name, true);
}

bool FieldImpl::updateDescription()
{
    return validateAndUpdateStringPropValue(common::descriptionStr(), m_description);
}

bool FieldImpl::updateDisplayName()
{
    return validateAndUpdateStringPropValue(common::displayNameStr(), m_displayName);
}

bool FieldImpl::updateSinceVersion()
{
    if (!validateSinglePropInstance(common::sinceVersionStr())) {
        return false;
    }

    auto parentMinVersion = 0U;
    if (getParent() != nullptr) {
        parentMinVersion = getParent()->getMinSinceVersion();
        assert(parentMinVersion <= getParent()->getMaxSinceVersion());
    }

    unsigned value = parentMinVersion;
    do {
        auto iter = m_props.find(common::sinceVersionStr());
        if (iter == m_props.end()) {
            break;
        }

        bool ok = false;
        value = common::strToUnsigned(iter->second, &ok);
        if (!ok) {
            reportUnexpectedPropertyValue(common::sinceVersionStr(), iter->second);
            return false;
        }
    } while (false);

    auto schemaVersion = protocol().schemaImpl().version();
    if (schemaVersion < value) {
        logError() << XmlWrap::logPrefix(m_node) <<
                      "The value of property \"" << common::sinceVersionStr() << "\" (" <<
                      value << ") cannot greater then version of the schema (" <<
                      schemaVersion << ").";
        return false;
    }

    if (value < parentMinVersion) {
        logError() << XmlWrap::logPrefix(m_node) <<
                      "The value of property \"" << common::sinceVersionStr() << "\" (" <<
                      value << ") cannot less then specified or inherited version of the parent (" <<
                      parentMinVersion << ").";
        return false;
    }

    setMinSinceVersion(value);
    setMaxSinceVersion(value);

    if (getParent() != nullptr) {
        getParent()->setMaxSinceVersion(std::max(getParent()->getMaxSinceVersion(), value));
    }
    return true;
}

bool FieldImpl::updateDeprecated()
{
    if (!validateSinglePropInstance(common::deprecatedStr())) {
        return false;
    }

    unsigned value = std::numeric_limits<unsigned>::max();
    do {
        auto iter = m_props.find(common::deprecatedStr());
        if (iter == m_props.end()) {
            break;
        }

        bool ok = false;
        value = common::strToUnsigned(iter->second, &ok);
        if (!ok) {
            reportUnexpectedPropertyValue(common::deprecatedStr(), iter->second);
            return false;
        }
    } while (false);

    if (value <= getMaxSinceVersion()) {
        logWarning() << XmlWrap::logPrefix(m_node) <<
                        "The value of \"" << common::deprecatedStr() << "\" property is not greater than "
                        "specified or inherited \"" << common::sinceVersionStr() << "\" one.";
    }

    setDeprecated(value);
    return true;
}

const FieldImpl::CreateMap& FieldImpl::createMap()
{
    static const CreateMap Map = {
        std::make_pair(
            common::intStr(),
            [](::xmlNodePtr n, ProtocolImpl& p)
            {
                return Ptr(new IntFieldImpl(n, p));
            }),
        std::make_pair(
            common::floatStr(),
            [](::xmlNodePtr n, ProtocolImpl& p)
            {
                return Ptr(new FloatFieldImpl(n, p));
            }),
        std::make_pair(
            common::bitfieldStr(),
            [](::xmlNodePtr n, ProtocolImpl& p)
            {
                return Ptr(new BitfieldFieldImpl(n, p));
            }),
        std::make_pair(
            common::bundleStr(),
            [](::xmlNodePtr n, ProtocolImpl& p)
            {
                return Ptr(new BundleFieldImpl(n, p));
            })
    };

    return Map;
}

} // namespace bbmp
