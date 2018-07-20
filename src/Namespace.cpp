#include "Namespace.h"

#include <cassert>
#include <algorithm>
#include <iterator>

#include "Generator.h"

namespace commsdsl2comms
{

const std::string& Namespace::name() const
{
    if (!m_dslObj.valid()) {
        return common::emptyString();
    }
    return m_dslObj.name();
}

bool Namespace::prepare()
{
    return
        prepareNamespaces() &&
        prepareFields() &&
        prepareInterfaces() &&
        prepareMessages() &&
        prepareFrames();
}

bool Namespace::writeInterfaces()
{
    return
        std::all_of(
            m_namespaces.begin(), m_namespaces.end(),
            [](auto& ptr)
            {
                return ptr->writeInterfaces();
            }) &&
        std::all_of(
            m_interfaces.begin(), m_interfaces.end(),
            [](auto& ptr)
            {
                return ptr->write();
            });
}

bool Namespace::writeMessages()
{
    return
        std::all_of(
            m_namespaces.begin(), m_namespaces.end(),
            [](auto& ptr)
            {
                return ptr->writeMessages();
            }) &&
        std::all_of(
            m_messages.begin(), m_messages.end(),
            [](auto& ptr)
            {
                return ptr->write();
            });
}

bool Namespace::writeFrames()
{
    return
        std::all_of(
            m_namespaces.begin(), m_namespaces.end(),
            [](auto& ptr)
            {
                return ptr->writeFrames();
            }) &&
        std::all_of(
            m_frames.begin(), m_frames.end(),
            [](auto& ptr)
            {
                return ptr->write();
    });
}

bool Namespace::writeFields()
{
    for (auto& n : m_namespaces) {
        if (!n->writeFields()) {
            return false;
        }
    }

    while (true) {
        std::size_t writtenCount = 0U;
        for (auto& f : m_accessedFields) {
            ++writtenCount;
            if (f.second) {
                continue; // already written
            }

            if (!f.first->writeFiles()) {
                return false;
            }

            f.second = true;
        }

        if (m_accessedFields.size() <= writtenCount) {
            break; // everything has been written
        }

        // new elements where introduced during writing
    }


    return true;
}

std::string Namespace::getDefaultOptions() const
{

    auto addFunc =
        [](const std::string& str, std::string& result)
        {
            if (str.empty()) {
                return;
            }

            if (!result.empty()) {
                result += '\n';
            }

            result += str;
        };

    std::string namespacesOpts;
    for (auto& n : m_namespaces) {
        addFunc(n->getDefaultOptions(), namespacesOpts);
    }

    std::string fieldsOpts;
    auto scope = m_generator.scopeForNamespace(externalRef());
    for (auto& f : m_fields) {
        auto iter = m_accessedFields.find(f.get());
        if (iter == m_accessedFields.end()) {
            continue;
        }

        addFunc(f->getDefaultOptions(scope), fieldsOpts);
    }

    std::string messagesOpts;
    for (auto& m : m_messages) {
        addFunc(m->getDefaultOptions(), messagesOpts);
    }
    std::string framesOpts;
    for (auto& f : m_frames) {
        addFunc(f->getDefaultOptions(), framesOpts);
    }

    if (!fieldsOpts.empty()) {
        static const std::string FieldsWrapTempl =
            "/// @brief Extra options for fields.\n"
            "struct field\n"
            "{\n"
            "    #^#FIELDS_OPTS#$#\n"
            "}; // struct field\n";

        common::ReplacementMap replacmenents;
        replacmenents.insert(std::make_pair("FIELDS_OPTS", fieldsOpts));
        fieldsOpts = common::processTemplate(FieldsWrapTempl, replacmenents);
    }

    if (!messagesOpts.empty()) {
        static const std::string MessageWrapTempl =
            "/// @brief Extra options for messages.\n"
            "struct message\n"
            "{\n"
            "    #^#MESSAGES_OPTS#$#\n"
            "}; // struct message\n";

        common::ReplacementMap replacmenents;
        replacmenents.insert(std::make_pair("MESSAGES_OPTS", messagesOpts));
        messagesOpts = common::processTemplate(MessageWrapTempl, replacmenents);
    }

    if (!framesOpts.empty()) {
        static const std::string FrameWrapTempl =
            "/// @brief Extra options for frames.\n"
            "struct frame\n"
            "{\n"
            "    #^#FRAMES_OPTS#$#\n"
            "}; // struct frame\n";

        common::ReplacementMap replacements;
        replacements.insert(std::make_pair("FRAMES_OPTS", framesOpts));
        framesOpts = common::processTemplate(FrameWrapTempl, replacements);
    }

    common::ReplacementMap replacmenents;
    replacmenents.insert(std::make_pair("NAMESPACE_NAME", name()));
    replacmenents.insert(std::make_pair("NAMESPACES_OPTS", std::move(namespacesOpts)));
    replacmenents.insert(std::make_pair("MESSAGES_OPTS", std::move(messagesOpts)));
    replacmenents.insert(std::make_pair("FIELDS_OPTS", std::move(fieldsOpts)));
    replacmenents.insert(std::make_pair("FRAMES_OPTS", std::move(framesOpts)));

    static const std::string Templ =
        "/// @brief Scope for extra options for fields and messages in the namespace.\n"
        "struct #^#NAMESPACE_NAME#$#\n"
        "{\n"
        "    #^#NAMESPACES_OPTS#$#\n"
        "    #^#FIELDS_OPTS#$#\n"
        "    #^#MESSAGES_OPTS#$#\n"
        "    #^#FRAMES_OPTS#$#\n"
        "};\n";

    static const std::string GlobalTempl =
        "#^#FIELDS_OPTS#$#\n"
        "#^#MESSAGES_OPTS#$#\n"
        "#^#FRAMES_OPTS#$#\n";

    auto* templ = &Templ;
    if (name().empty()) {
        templ = &GlobalTempl;
    }

    return common::processTemplate(*templ, replacmenents);
}

Namespace::MessagesAccessList Namespace::getAllMessages() const
{
    MessagesAccessList result;
    for (auto& n : m_namespaces) {
        auto list = n->getAllMessages();
        result.insert(result.end(), list.begin(), list.end());
    }

    result.reserve(result.size() + m_messages.size());
    for (auto& m : m_messages) {
        result.emplace_back(m.get());
    }

    return result;
}

bool Namespace::hasInterfaceDefined()
{
    bool defined =
        std::any_of(
            m_namespaces.begin(), m_namespaces.end(),
            [](auto& n)
            {
                return n->hasInterfaceDefined();
            });

    if (defined) {
        return true;
    }

    return !m_interfaces.empty();
}

const Field* Namespace::findMessageIdField() const
{
    for (auto& f : m_fields) {
        if (f->semanticType() != commsdsl::Field::SemanticType::MessageId) {
            continue;
        }

        if (f->kind() != commsdsl::Field::Kind::Enum) {
            assert(!"Unexpected field");
            return nullptr;
        }

        return f.get();
    }

    for (auto& n : m_namespaces) {
        auto ptr = n->findMessageIdField();
        if (ptr != nullptr) {
            return ptr;
        }
    }

    return nullptr;
}

const Field* Namespace::findField(const std::string& externalRef, bool record)
{
    assert(!externalRef.empty());
    auto pos = externalRef.find_first_of('.');
    std::string nsName;
    if (pos != std::string::npos) {
        nsName.assign(externalRef.begin(), externalRef.begin() + pos);
    }

    if (nsName.empty()) {
        auto fieldIter =
            std::lower_bound(
                m_fields.begin(), m_fields.end(), externalRef,
                [&externalRef](auto& f, auto& n)
                {
                    return f->name() < n;
                });

        if ((fieldIter == m_fields.end()) || ((*fieldIter)->name() != externalRef)) {
            return nullptr;
        }

        if (record) {
            recordAccessedField(fieldIter->get());
        }
        return fieldIter->get();
    }

    auto nsIter =
        std::lower_bound(
            m_namespaces.begin(), m_namespaces.end(), nsName,
            [](auto& ns, const std::string& n)
            {
                return ns->name() < n;
            });

    if ((nsIter == m_namespaces.end()) || ((*nsIter)->name() != nsName)) {
        return nullptr;
    }

    auto fromPos = 0U;
    if (pos != std::string::npos) {
        fromPos = pos + 1U;
    }
    std::string remStr(externalRef, fromPos);
    return (*nsIter)->findField(remStr, record);
}

bool Namespace::anyInterfaceHasVersion() const
{
    bool hasVersion =
        std::any_of(
            m_interfaces.begin(), m_interfaces.end(),
            [](auto& i)
            {
                return i->hasVersion();
            });

    if (hasVersion) {
        return true;
    }

    return
        std::any_of(
            m_namespaces.begin(), m_namespaces.end(),
            [](auto& n)
            {
                return n->anyInterfaceHasVersion();
    });
}

common::StringsList Namespace::pluginCommonSources() const
{
    common::StringsList result;
    std::string prefix = name();
    if (!prefix.empty()) {
        prefix += '/';
    }

    for (auto& n : m_namespaces) {
        auto subResult = n->pluginCommonSources();
        result.reserve(result.size() + subResult.size());
        std::transform(
            subResult.begin(), subResult.end(), std::back_inserter(result),
            [&prefix](const std::string& s)
            {
                return prefix + s;
            });
    }

    result.reserve(result.size() + m_accessedFields.size() + m_messages.size());
    std::transform(
        m_accessedFields.begin(), m_accessedFields.end(), std::back_inserter(result),
        [&prefix](auto& f)
        {
            return prefix + common::fieldStr() + '/' + common::nameToClassCopy(f.first->name()) + common::srcSuffix();
        });

//    for (auto& m : m_messages) {
//        if (!m->doesExist()) {
//            continue;
//        }

//        result.push_back(prefix + common::messageStr() + '/' + common::nameToClassCopy(m->name()) + common::srcSuffix());
//    }

    for (auto& i : m_interfaces) {
        result.push_back(prefix + common::nameToClassCopy(i->name()) + common::srcSuffix());
    }


    return result;
}

std::string Namespace::externalRef() const
{
    if (m_dslObj.valid()) {
        return m_dslObj.externalRef();
    }

    return common::emptyString();
}

bool Namespace::addDefaultInterface()
{
    assert((m_interfaces.empty()) || (!m_interfaces.front()->name().empty()));
    auto interface = createInterface(m_generator, commsdsl::Interface(nullptr));
    if (!interface->prepare()) {
        return false;
    }

    m_interfaces.insert(m_interfaces.begin(), std::move(interface));
    return true;
}


bool Namespace::prepareNamespaces()
{
    if (!m_dslObj.valid()) {
        return true;
    }

    auto namespaces = m_dslObj.namespaces();
    m_namespaces.reserve(namespaces.size());
    for (auto& n : namespaces) {
        auto ptr = createNamespace(m_generator, n);
        assert(ptr);
        if (!ptr->prepare()) {
            return false;
        }

        m_namespaces.push_back(std::move(ptr));
    }

    return true;
}

bool Namespace::prepareFields()
{
    if (!m_dslObj.valid()) {
        return true;
    }

    auto fields = m_dslObj.fields();
    m_fields.reserve(fields.size());
    for (auto& dslObj : fields) {
        auto ptr = Field::create(m_generator, dslObj);
        assert(ptr);
        if (!ptr->prepare(0U)) {
            return false;
        }

        m_fields.push_back(std::move(ptr));
    }

    return true;
}

bool Namespace::prepareInterfaces()
{
    if (!m_dslObj.valid()) {
        return true;
    }

    auto interfaces = m_dslObj.interfaces();
    m_interfaces.reserve(interfaces.size());
    for (auto& dslObj : interfaces) {
        auto ptr = createInterface(m_generator, dslObj);
        assert(ptr);
        if (!ptr->prepare()) {
            return false;
        }

        m_interfaces.push_back(std::move(ptr));
    }

    return true;
}

bool Namespace::prepareMessages()
{
    if (!m_dslObj.valid()) {
        return true;
    }

    auto messages = m_dslObj.messages();
    m_messages.reserve(messages.size());
    for (auto& dslObj : messages) {
        auto ptr = createMessage(m_generator, dslObj);
        assert(ptr);
        if (!ptr->prepare()) {
            return false;
        }

        m_messages.push_back(std::move(ptr));
    }

    return true;
}

bool Namespace::prepareFrames()
{
    if (!m_dslObj.valid()) {
        return true;
    }

    auto frames = m_dslObj.frames();
    m_frames.reserve(frames.size());
    for (auto& dslObj : frames) {
        auto ptr = createFrame(m_generator, dslObj);
        assert(ptr);
        if (!ptr->prepare()) {
            return false;
        }

        m_frames.push_back(std::move(ptr));
    }

    return true;
}

void Namespace::recordAccessedField(const Field* field)
{
    auto iter = m_accessedFields.find(field);
    if (iter != m_accessedFields.end()) {
        return;
    }

    m_accessedFields.insert(std::make_pair(field, false));
}

}
