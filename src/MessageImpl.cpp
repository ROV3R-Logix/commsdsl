#include "MessageImpl.h"

#include <cassert>
#include <limits>
#include <algorithm>
#include <set>
#include <numeric>

#include "ProtocolImpl.h"
#include "NamespaceImpl.h"
#include "common.h"
#include "OptionalFieldImpl.h"

namespace bbmp
{

namespace
{

const XmlWrap::NamesList& messageSupportedTypes()
{
    static const XmlWrap::NamesList Names = FieldImpl::supportedTypes();
    return Names;
}


} // namespace

MessageImpl::MessageImpl(::xmlNodePtr node, ProtocolImpl& protocol)
  : m_node(node),
    m_protocol(protocol),
    m_name(&common::emptyString()),
    m_displayName(&common::emptyString()),
    m_description(&common::emptyString())
{
}

bool MessageImpl::parse()
{
    m_props = XmlWrap::parseNodeProps(m_node);

    if (!XmlWrap::parseChildrenAsProps(m_node, commonProps(), m_protocol.logger(), m_props)) {
        return false;
    }

    bool result =
        updateName() &&
        updateDisplayName() &&
        updateDescription() &&
        updateId() &&
        updateOrder() &&
        updateVersions() &&
        copyFields() &&
        updateFields();

    if (!result) {
        return false;
    }

    XmlWrap::NamesList expectedProps = commonProps();
    auto unknownAttrs = XmlWrap::getUnknownProps(m_node, expectedProps);
    if (!unknownAttrs.empty()) {
        logWarning() << XmlWrap::logPrefix(m_node) <<
            "The \"" << name() << "\" message contains unexpected XML attributes";
    }

    XmlWrap::NamesList expectedChildren = commonProps();
    const XmlWrap::NamesList& supportedFieldTypes = messageSupportedTypes();
    expectedChildren.insert(expectedChildren.end(), supportedFieldTypes.begin(), supportedFieldTypes.end());
    expectedChildren.push_back(common::fieldsStr());
    auto unknownChildren = XmlWrap::getUnknownChildren(m_node, expectedChildren);
    if (!unknownChildren.empty()) {
        logWarning() << XmlWrap::logPrefix(m_node) <<
            "The \"" << name() << "\" message contains unexpected XML children elements";
    }
    return true;
}

const std::string& MessageImpl::name() const
{
    assert(m_name != nullptr);
    return *m_name;
}

const std::string& MessageImpl::displayName() const
{
    assert(m_displayName != nullptr);
    return *m_displayName;
}

const std::string& MessageImpl::description() const
{
    assert(m_description != nullptr);
    return *m_description;
}

std::size_t MessageImpl::minLength() const
{
    return
        std::accumulate(
            m_fields.begin(), m_fields.end(), 0U,
                [this](std::size_t soFar, auto& elem)
                {
                    if (this->getSinceVersion() < elem->getSinceVersion()) {
                        return soFar;
                    }

                    return soFar + elem->minLength();
                });
}

std::size_t MessageImpl::maxLength() const
{
    static const auto MaxLen = std::numeric_limits<std::size_t>::max();
    std::size_t soFar = 0U;
    for (auto& f : m_fields) {
        auto fMaxLen = f->maxLength();
        if ((MaxLen - fMaxLen) <= soFar) {
            return MaxLen;
        }

        soFar += fMaxLen;
    }
    return soFar;
}

MessageImpl::FieldsList MessageImpl::fieldsList() const
{
    FieldsList result;
    result.reserve(m_fields.size());
    std::transform(
        m_fields.begin(), m_fields.end(), std::back_inserter(result),
        [](auto& f)
        {
            return Field(f.get());
        });
    return result;
}

std::string MessageImpl::externalRef() const
{
    assert(getParent() != nullptr);
    assert(getParent()->objKind() == ObjKind::Namespace);

    auto& ns = static_cast<const bbmp::NamespaceImpl&>(*getParent());
    auto nsRef = ns.externalRef();
    if (nsRef.empty()) {
        return name();
    }

    return nsRef + '.' + name();
}

LogWrapper MessageImpl::logError() const
{
    return bbmp::logError(m_protocol.logger());
}

LogWrapper MessageImpl::logWarning() const
{
    return bbmp::logWarning(m_protocol.logger());
}

LogWrapper MessageImpl::logInfo() const
{
    return bbmp::logInfo(m_protocol.logger());
}

Object::ObjKind MessageImpl::objKindImpl() const
{
    return ObjKind::Message;
}


bool MessageImpl::validateSinglePropInstance(const std::string& str, bool mustHave)
{
    return XmlWrap::validateSinglePropInstance(m_node, m_props, str, m_protocol.logger(), mustHave);
}

bool MessageImpl::validateAndUpdateStringPropValue(
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

void MessageImpl::reportUnexpectedPropertyValue(const std::string& propName, const std::string& propValue)
{
    XmlWrap::reportUnexpectedPropertyValue(m_node, name(), propName, propValue, m_protocol.logger());
}

const XmlWrap::NamesList& MessageImpl::commonProps()
{
    static const XmlWrap::NamesList CommonNames = {
        common::nameStr(),
        common::idStr(),
        common::displayNameStr(),
        common::descriptionStr(),
        common::sinceVersionStr(),
        common::deprecatedStr(),
        common::removedStr(),
        common::copyFieldsFromStr(),
        common::orderStr()
    };

    return CommonNames;
}

bool MessageImpl::updateName()
{
    assert(m_name != nullptr);
    bool mustHave = m_name->empty();
    if (!validateAndUpdateStringPropValue(common::nameStr(), m_name, mustHave)) {
        return false;
    }

    if (!common::isValidName(*m_name)) {
        logError() << XmlWrap::logPrefix(getNode()) <<
                      "Invalid value for name property \"" << m_name << "\".";
        return false;
    }

    return true;
}

bool MessageImpl::updateDescription()
{
    return validateAndUpdateStringPropValue(common::descriptionStr(), m_description);
}

bool MessageImpl::updateDisplayName()
{
    return validateAndUpdateStringPropValue(common::displayNameStr(), m_displayName);
}

bool MessageImpl::updateId()
{
    if (!validateSinglePropInstance(common::idStr(), true)) {
        return false;
    }

    auto iter = m_props.find(common::idStr());
    std::intmax_t val = 0;
    if (m_protocol.strToEnumValue(iter->second, val)) {
        m_id = static_cast<decltype(m_id)>(val);
        return true;
    }

    bool ok = false;
    m_id = common::strToUintMax(iter->second, &ok);
    if (!ok) {
        reportUnexpectedPropertyValue(common::idStr(), iter->second);
        return false;
    }

    return true;
}

bool MessageImpl::updateOrder()
{
    if (!validateSinglePropInstance(common::orderStr())) {
        return false;
    }

    auto iter = m_props.find(common::orderStr());
    if (iter == m_props.end()) {
        assert(m_order == 0U);
        return true;
    }

    bool ok = false;
    m_order = common::strToUnsigned(iter->second, &ok);
    if (!ok) {
        reportUnexpectedPropertyValue(common::orderStr(), iter->second);
        return false;
    }

    return true;
}

bool MessageImpl::updateVersions()
{
    if (!validateSinglePropInstance(common::sinceVersionStr())) {
        return false;
    }

    if (!validateSinglePropInstance(common::deprecatedStr())) {
        return false;
    }

    if (!validateSinglePropInstance(common::removedStr())) {
        return false;
    }

    assert(getParent() != nullptr);
    assert(getParent()->objKind() == ObjKind::Namespace);

    unsigned sinceVersion = 0U;
    unsigned deprecated = Protocol::notYetDeprecated();
    if (!XmlWrap::getAndCheckVersions(m_node, name(), m_props, sinceVersion, deprecated, m_protocol)) {
        return false;
    }

    bool deprecatedRemoved = false;
    do {
        auto deprecatedRemovedIter = m_props.find(common::removedStr());
        if (deprecatedRemovedIter == m_props.end()) {
            break;
        }

        bool ok = false;
        deprecatedRemoved = common::strToBool(deprecatedRemovedIter->second, &ok);
        if (!ok) {
            reportUnexpectedPropertyValue(common::removedStr(), deprecatedRemovedIter->second);
            return false;
        }

        if (!deprecatedRemoved) {
            break;
        }

        if (deprecated == bbmp::Protocol::notYetDeprecated()) {
            logWarning() << XmlWrap::logPrefix(getNode()) <<
                "Property \"" << common::removedStr() << "\" is not applicable to "
                "non deprecated fields";
        }
    } while (false);

    setSinceVersion(sinceVersion);
    setDeprecated(deprecated);
    setDeprecatedRemoved(deprecatedRemoved);
    return true;
}

bool MessageImpl::copyFields()
{
    if (!validateSinglePropInstance(common::copyFieldsFromStr())) {
        return false;
    }

    auto iter = props().find(common::copyFieldsFromStr());
    if (iter == props().end()) {
        return true;
    }

    auto* otherMsg = m_protocol.findMessage(iter->second);
    if (otherMsg == nullptr) {
        logError() << XmlWrap::logPrefix(getNode()) <<
            "Invalid reference to other message \"" << iter->second << "\".";
        return false;
    }

    cloneFieldsFrom(*otherMsg);

    if (!m_fields.empty()) {
        m_fields.erase(
            std::remove_if(
                m_fields.begin(), m_fields.end(),
                [this](auto& elem)
                {
                    return
                        (elem->isDeprecatedRemoved()) &&
                        (elem->getDeprecated() <= this->getSinceVersion());
                }),
            m_fields.end());

        for (auto& m : m_fields) {
            m->setSinceVersion(std::max(getSinceVersion(), m->getSinceVersion()));
        }
    }
    return true;
}

bool MessageImpl::updateFields()
{
    do {
        auto fieldsNodes = XmlWrap::getChildren(getNode(), common::fieldsStr());
        if (1U < fieldsNodes.size()) {
            logError() << XmlWrap::logPrefix(getNode()) <<
                          "Only single \"" << common::fieldsStr() << "\" child element is "
                          "supported for \"" << common::messageStr() << "\".";
            return false;
        }

        auto fieldsTypes = XmlWrap::getChildren(getNode(), messageSupportedTypes());
        if ((!fieldsNodes.empty()) && (!fieldsTypes.empty())) {
            logError() << XmlWrap::logPrefix(getNode()) <<
                          "The \"" << common::messageStr() << "\" element does not support "
                          "list of stand alone fields as child elements together with \"" <<
                          common::fieldsStr() << "\" child element.";
            return false;
        }

        if ((fieldsNodes.empty()) && (fieldsTypes.empty())) {
            break;
        }

        if ((0U < fieldsTypes.size())) {
            assert(0U == fieldsNodes.size());
            auto allChildren = XmlWrap::getChildren(getNode());
            if (allChildren.size() != fieldsTypes.size()) {
                logError() << XmlWrap::logPrefix(getNode()) <<
                              "The field types of \"" << common::messageStr() <<
                              "\" must be defined inside \"<" << common::fieldsStr() << ">\" child element "
                              "when there are other property describing children.";
                return false;
            }
        }

        if (0U < fieldsNodes.size()) {
            assert(0U == fieldsTypes.size());
            fieldsTypes = XmlWrap::getChildren(fieldsNodes.front());
            auto cleanMemberFieldsTypes = XmlWrap::getChildren(fieldsNodes.front(), messageSupportedTypes());
            if (cleanMemberFieldsTypes.size() != fieldsTypes.size()) {
                logError() << XmlWrap::logPrefix(fieldsNodes.front()) <<
                    "The \"" << common::fieldsStr() << "\" child node of \"" <<
                    common::messageStr() << "\" element must contain only supported field types.";
                return false;
            }

            // fieldsTypes is updated with the list from <fields>
        }

        m_fields.reserve(m_fields.size() + fieldsTypes.size());
        for (auto* fNode : fieldsTypes) {
            std::string fKind(reinterpret_cast<const char*>(fNode->name));
            auto field = FieldImpl::create(fKind, fNode, m_protocol);
            if (!field) {
                assert(!"Internal error");
                logError() << XmlWrap::logPrefix(getNode()) <<
                      "Internal error, failed to create objects for member fields.";
                return false;
            }

            field->setParent(this);
            if (!field->parse()) {
                return false;
            }

            do {
                if (field->kind() != FieldImpl::Kind::Optional) {
                    break;
                }

                auto& optField = static_cast<OptionalFieldImpl&>(*field);
                auto& cond = optField.cond();
                if (!cond) {
                    break;
                }

                if (!cond->verify(m_fields, field->getNode(), m_protocol.logger())) {
                    return false;
                }

            } while (false);

            m_fields.push_back(std::move(field));
        }

        if (!FieldImpl::validateMembersNames(m_fields, m_protocol.logger())) {
            return false;
        }

    } while (false);

    return true;
}

void MessageImpl::cloneFieldsFrom(const MessageImpl& other)
{
    m_fields.reserve(other.m_fields.size());
    for (auto& f : other.m_fields) {
        m_fields.push_back(f->clone());
    }
}

} // namespace bbmp
