#pragma once

#include <string>
#include <memory>
#include <vector>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include "bbmp/Protocol.h"
#include "bbmp/ErrorLevel.h"
#include "bbmp/Schema.h"
#include "Logger.h"
#include "SchemaImpl.h"
#include "NamespaceImpl.h"

namespace bbmp
{

class ProtocolImpl
{
public:
    using ErrorReportFunction = Protocol::ErrorReportFunction;
    using NamespacesList = Protocol::NamespacesList;
    using MessagesList = Protocol::MessagesList;
    using ExtraPrefixes = std::vector<std::string>;
    using PlatformsList = Protocol::PlatformsList;

    ProtocolImpl();
    bool parse(const std::string& input);
    bool validate();

    Schema schema() const;

    SchemaImpl& schemaImpl();
    const SchemaImpl& schemaImpl() const;


    void setErrorReportCallback(ErrorReportFunction&& cb)
    {
        m_errorReportCb = std::move(cb);
    }

    Logger& logger() const
    {
        return m_logger;
    }


    NamespacesList namespacesList() const;

    const FieldImpl* findField(const std::string& ref, bool checkRef = true) const;

    const MessageImpl* findMessage(const std::string& ref, bool checkRef = true) const;

    bool strToEnumValue(const std::string& ref, std::intmax_t& val, bool checkRef = true) const;

    MessagesList allMessages() const;

    void addExpectedExtraPrefix(const std::string& value)
    {
        m_extraPrefixes.push_back(value);
    }

    const ExtraPrefixes& extraElementPrefixes() const
    {
        return m_extraPrefixes;
    }

    const PlatformsList& platforms() const
    {
        return m_platforms;
    }

private:
    struct XmlDocFree
    {
        void operator()(::xmlDocPtr p) const
        {
            ::xmlFreeDoc(p);
        }
    };

    using XmlDocPtr = std::unique_ptr<::xmlDoc, XmlDocFree>;
    using DocsList = std::vector<XmlDocPtr>;
    using SchemaImplPtr = std::unique_ptr<SchemaImpl>;
    using NamespacesMap = NamespaceImpl::NamespacesMap;

    static void cbXmlErrorFunc(void* userData, xmlErrorPtr err);
    void handleXmlError(xmlErrorPtr err);
    bool validateDoc(::xmlDocPtr doc);
    bool validateSchema(::xmlNodePtr node);
    bool validatePlatforms(::xmlNodePtr root);
    bool validateSinglePlatform(::xmlNodePtr node);
    bool validateNamespaces(::xmlNodePtr root);
    bool validateAllMessages();
    const NamespaceImpl* getNsFromPath(const std::string& ref, bool checkRef, std::string& remName) const;

    LogWrapper logError() const;
    LogWrapper logWarning() const;

    ErrorReportFunction m_errorReportCb;
    DocsList m_docs;
    bool m_validated = false;
    ErrorLevel m_minLevel = ErrorLevel_Info;
    mutable Logger m_logger;
    SchemaImplPtr m_schema;
    NamespacesMap m_namespaces;
    ExtraPrefixes m_extraPrefixes;
    PlatformsList m_platforms;
};

} // namespace bbmp
