//
// Copyright 2018 - 2019 (C). Alex Robenko. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Plugin.h"

#include <cassert>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "Generator.h"

namespace ba = boost::algorithm;
namespace bf = boost::filesystem;

namespace commsdsl2comms
{

namespace
{

const std::string ProtSuffix("Protocol");
const std::string PluginSuffix("Plugin");

} // namespace

bool Plugin::prepare()
{
    m_framePtr = m_generator.findFrame(m_frame);
    if (m_framePtr == nullptr) {
        m_generator.logger().error("Frame \"" + m_frame + "\" hasn't been defined.");
        return false;
    }

    m_interfacePtr = m_generator.findInterface(m_interface);
    if (m_interfacePtr == nullptr) {
        m_generator.logger().error("Interface \"" + m_interface + "\" hasn't been defined.");
        return false;
    }

    return true;
}

bool Plugin::write()
{
    return
        writeProtocolHeader() &&
        writeProtocolSrc() &&
        writePluginHeader() &&
        writePluginSrc() &&
        writePluginJson() &&
        writePluginConfig();
}

const std::string& Plugin::adjustedName() const
{
    auto* nameToUse = &m_name;
    if (nameToUse->empty()) {
        nameToUse = &m_generator.schemaName();
    }
    return *nameToUse;
}

bool Plugin::writeProtocolHeader()
{
    auto startInfo = m_generator.startProtocolPluginHeaderWrite(protClassName());
    auto& filePath = startInfo.first;
    auto& className = startInfo.second;

    if (filePath.empty()) {
        // Skipping generation
        return true;
    }

    static const std::string Templ =
        "#pragma once\n\n"
        "#include \"comms_champion/Protocol.h\"\n\n"
        "#^#BEGIN_NAMESPACE#$#\n"
        "class #^#CLASS_NAME#$#Impl;\n"
        "class #^#CLASS_NAME#$# : public comms_champion::Protocol\n"
        "{\n"
        "public:\n"
        "    #^#CLASS_NAME#$#();\n"
        "    virtual ~#^#CLASS_NAME#$#();\n\n"
        "protected:\n"
        "    virtual const QString& nameImpl() const override;\n"
        "    virtual MessagesList readImpl(const comms_champion::DataInfo& dataInfo, bool final) override;\n"
        "    virtual comms_champion::DataInfoPtr writeImpl(comms_champion::Message& msg) override;\n"
        "    virtual MessagesList createAllMessagesImpl() override;\n"
        "    virtual comms_champion::MessagePtr createMessageImpl(const QString& idAsString, unsigned idx) override;\n"
        "    virtual UpdateStatus updateMessageImpl(comms_champion::Message& msg) override;\n"
        "    virtual comms_champion::MessagePtr cloneMessageImpl(const comms_champion::Message& msg) override;\n"
        "    virtual comms_champion::MessagePtr createInvalidMessageImpl() override;\n"
        "    virtual comms_champion::MessagePtr createRawDataMessageImpl() override;\n"
        "    virtual comms_champion::MessagePtr createExtraInfoMessageImpl() override;\n\n"
        "private:\n"
        "    std::unique_ptr<#^#CLASS_NAME#$#Impl> m_pImpl;\n"
        "};\n\n"
        "#^#END_NAMESPACE#$#\n"
        "#^#APPEND#$#\n"
    ;

    auto namespaces = m_generator.namespacesForPluginDef(className);

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("CLASS_NAME", className));
    replacements.insert(std::make_pair("BEGIN_NAMESPACE", std::move(namespaces.first)));
    replacements.insert(std::make_pair("END_NAMESPACE", std::move(namespaces.second)));
    replacements.insert(std::make_pair("APPEND", m_generator.getExtraAppendForPluginHeaderInPlugin(protClassName())));

    std::string str = common::processTemplate(Templ, replacements);

    std::ofstream stream(filePath);
    if (!stream) {
        m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
        return false;
    }
    stream << str;

    if (!stream.good()) {
        m_generator.logger().error("Failed to write \"" + filePath + "\".");
        return false;
    }

    return true;
}

bool Plugin::writeProtocolSrc()
{
    auto startInfo = m_generator.startProtocolPluginSrcWrite(protClassName());
    auto& filePath = startInfo.first;
    auto& className = startInfo.second;

    if (filePath.empty()) {
        // Skipping generation
        return true;
    }

    static const std::string Templ =
    "#include \"#^#CLASS_NAME#$#.h\"\n\n"
    "#include \"comms_champion/ProtocolBase.h\"\n"
    "#^#INTERFACE_INC#$#\n"
    "#include \"#^#FRAME_HEADER#$#\"\n"
    "#include \"#^#TRANSPORT_MESSAGE_HEADER#$#\"\n\n"
    "namespace cc = comms_champion;\n\n"
    "#^#BEGIN_NAMESPACE#$#\n"
    "class #^#CLASS_NAME#$#Impl : public\n"
    "    cc::ProtocolBase<\n"
    "        #^#FRAME#$##^#INTERFACE_TEMPL_PARAM#$#,\n"
    "        #^#FRAME#$#TransportMessage#^#INTERFACE_TEMPL_PARAM#$#\n"
    "    >\n"
    "{\n"
    "    using Base =\n"
    "        cc::ProtocolBase<\n"
    "            #^#FRAME#$##^#INTERFACE_TEMPL_PARAM#$#,\n"
    "            #^#FRAME#$#TransportMessage#^#INTERFACE_TEMPL_PARAM#$#\n"
    "        >;\n"
    "public:\n"
    "    friend class #^#PROT_NAMESPACE#$#::cc_plugin::plugin::#^#CLASS_NAME#$#;\n\n"
    "    #^#CLASS_NAME#$#Impl() = default;\n"
    "    virtual ~#^#CLASS_NAME#$#Impl() = default;\n\n"
    "protected:\n"
    "    virtual const QString& nameImpl() const override\n"
    "    {\n"
    "        static const QString Str(\"#^#PROT_NAME#$#\");\n"
    "        return Str;\n"
    "    }\n\n"
    "    using Base::createInvalidMessageImpl;\n"
    "    using Base::createRawDataMessageImpl;\n"
    "    using Base::createExtraInfoMessageImpl;\n"
    "};\n\n"
    "#^#CLASS_NAME#$#::#^#CLASS_NAME#$#()\n"
    "  : m_pImpl(new #^#CLASS_NAME#$#Impl())\n"
    "{\n"
    "}\n\n"
    "#^#CLASS_NAME#$#::~#^#CLASS_NAME#$#() = default;\n\n"
    "const QString& #^#CLASS_NAME#$#::nameImpl() const\n"
    "{\n"
    "    return m_pImpl->name();\n"
    "}\n\n"
    "#^#CLASS_NAME#$#::MessagesList #^#CLASS_NAME#$#::readImpl(const cc::DataInfo& dataInfo, bool final)\n"
    "{\n"
    "    return m_pImpl->read(dataInfo, final);\n"
    "}\n\n"
    "cc::DataInfoPtr #^#CLASS_NAME#$#::writeImpl(cc::Message& msg)\n"
    "{\n"
    "    return m_pImpl->write(msg);\n"
    "}\n\n"
    "#^#CLASS_NAME#$#::MessagesList #^#CLASS_NAME#$#::createAllMessagesImpl()\n"
    "{\n"
    "    return m_pImpl->createAllMessages();\n"
    "}\n\n"
    "cc::MessagePtr #^#CLASS_NAME#$#::createMessageImpl(const QString& idAsString, unsigned idx)\n"
    "{\n"
    "    return static_cast<cc::Protocol*>(m_pImpl.get())->createMessage(idAsString, idx);\n"
    "}\n\n"
    "#^#CLASS_NAME#$#::UpdateStatus #^#CLASS_NAME#$#::updateMessageImpl(cc::Message& msg)\n"
    "{\n"
    "    return m_pImpl->updateMessage(msg);\n"
    "}\n\n"
    "cc::MessagePtr #^#CLASS_NAME#$#::cloneMessageImpl(const cc::Message& msg)\n"
    "{\n"
    "    return m_pImpl->cloneMessage(msg);\n"
    "}\n\n"
    "cc::MessagePtr #^#CLASS_NAME#$#::createInvalidMessageImpl()\n"
    "{\n"
    "    return m_pImpl->createInvalidMessageImpl();\n"
    "}\n\n"
    "cc::MessagePtr #^#CLASS_NAME#$#::createRawDataMessageImpl()\n"
    "{\n"
    "    return m_pImpl->createRawDataMessageImpl();\n"
    "}\n\n"
    "cc::MessagePtr #^#CLASS_NAME#$#::createExtraInfoMessageImpl()\n"
    "{\n"
    "    return m_pImpl->createExtraInfoMessageImpl();\n"
    "}\n\n"
    "#^#END_NAMESPACE#$#\n"
    "#^#APPEND#$#\n";

    assert(m_framePtr != nullptr);
    auto namespaces = m_generator.namespacesForPluginDef(className);
    auto frameHeader = m_generator.headerfileForFrameInPlugin(m_framePtr->externalRef(), false);
    assert(ba::ends_with(frameHeader, common::headerSuffix()));
    auto transportMsgHeader = frameHeader;
    auto& suffix = common::transportMessageSuffixStr();
    auto insertIter = transportMsgHeader.begin() + (frameHeader.size() - common::headerSuffix().size());
    transportMsgHeader.insert(insertIter, suffix.begin(), suffix.end());

    const auto* protName = &m_name;
    if (protName->empty()) {
        protName = &m_generator.schemaName();
    }

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("CLASS_NAME", std::move(className)));
    replacements.insert(std::make_pair("PROT_NAMESPACE", m_generator.mainNamespace()));
    replacements.insert(std::make_pair("BEGIN_NAMESPACE", std::move(namespaces.first)));
    replacements.insert(std::make_pair("END_NAMESPACE", std::move(namespaces.second)));
    replacements.insert(std::make_pair("FRAME_HEADER", std::move(frameHeader)));
    replacements.insert(std::make_pair("TRANSPORT_MESSAGE_HEADER", std::move(transportMsgHeader)));
    replacements.insert(std::make_pair("FRAME", m_generator.scopeForFrameInPlugin(m_framePtr->externalRef())));
    replacements.insert(std::make_pair("PROT_NAME", *protName));
    replacements.insert(std::make_pair("APPEND", m_generator.getExtraAppendForPluginSrcInPlugin(protClassName())));

    auto* defaultInterface = m_generator.getDefaultInterface();
    if (defaultInterface == nullptr) {
        assert(m_interfacePtr != nullptr);
        replacements.insert(std::make_pair("INTERFACE_TEMPL_PARAM", '<' + m_generator.scopeForInterfaceInPlugin(m_interfacePtr->externalRef()) + '>'));
        replacements.insert(std::make_pair("INTERFACE_INC", "#include " + m_generator.headerfileForInterfaceInPlugin(m_interfacePtr->externalRef(), true)));
    }

    std::string str = common::processTemplate(Templ, replacements);

    std::ofstream stream(filePath);
    if (!stream) {
        m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
        return false;
    }
    stream << str;

    if (!stream.good()) {
        m_generator.logger().error("Failed to write \"" + filePath + "\".");
        return false;
    }

    return true;
}

bool Plugin::writePluginHeader()
{
    auto startInfo = m_generator.startProtocolPluginHeaderWrite(pluginClassName());
    auto& filePath = startInfo.first;
    auto& className = startInfo.second;

    if (filePath.empty()) {
        // Skipping generation
        return true;
    }

    static const std::string Templ =
        "#pragma once\n\n"
        "#include <QtCore/QObject>\n"
        "#include <QtCore/QtPlugin>\n"
        "#include \"comms_champion/Plugin.h\"\n\n"
        "#^#BEGIN_NAMESPACE#$#\n"
        "class #^#CLASS_NAME#$# : public comms_champion::Plugin\n"
        "{\n"
        "    Q_OBJECT\n"
        "    Q_PLUGIN_METADATA(IID \"#^#ID#$#\" FILE \"#^#ORIG_CLASS_NAME#$#.json\")\n"
        "    Q_INTERFACES(comms_champion::Plugin)\n\n"
        "public:\n"
        "    #^#CLASS_NAME#$#();\n"
        "    virtual ~#^#CLASS_NAME#$#();\n"
        "};\n\n"
        "#^#END_NAMESPACE#$#\n"
        "#^#APPEND#$#\n";

    auto namespaces = m_generator.namespacesForPluginDef(className);

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("CLASS_NAME", className));
    replacements.insert(std::make_pair("ORIG_CLASS_NAME", common::nameToClassCopy(pluginClassName())));
    replacements.insert(std::make_pair("BEGIN_NAMESPACE", std::move(namespaces.first)));
    replacements.insert(std::make_pair("END_NAMESPACE", std::move(namespaces.second)));
    replacements.insert(std::make_pair("ID", pluginId()));
    replacements.insert(std::make_pair("APPEND", m_generator.getExtraAppendForPluginHeaderInPlugin(pluginClassName())));

    std::string str = common::processTemplate(Templ, replacements);

    std::ofstream stream(filePath);
    if (!stream) {
        m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
        return false;
    }
    stream << str;

    if (!stream.good()) {
        m_generator.logger().error("Failed to write \"" + filePath + "\".");
        return false;
    }

    return true;
}

bool Plugin::writePluginSrc()
{
    auto startInfo = m_generator.startProtocolPluginSrcWrite(pluginClassName());
    auto& filePath = startInfo.first;
    auto& className = startInfo.second;

    if (filePath.empty()) {
        // Skipping generation
        return true;
    }

    static const std::string Templ =
        "#include \"#^#CLASS_NAME#$#.h\"\n\n"
        "#include \"#^#PROTOCOL_CLASS_NAME#$#.h\"\n\n"
        "namespace cc = comms_champion;\n\n"
        "#^#BEGIN_NAMESPACE#$#\n"
        "#^#CLASS_NAME#$#::#^#CLASS_NAME#$#()\n"
        "{\n"
        "    pluginProperties()\n"
        "        .setProtocolCreateFunc(\n"
        "            []() -> cc::ProtocolPtr\n"
        "            {\n"
        "                return cc::ProtocolPtr(new #^#PROTOCOL_CLASS_NAME#$#());\n"
        "            });\n"
        "}\n\n"
        "#^#CLASS_NAME#$#::~#^#CLASS_NAME#$#() = default;\n\n"
        "#^#END_NAMESPACE#$#\n"
        "#^#APPEND#$#\n";

    auto namespaces = m_generator.namespacesForPluginDef(className);

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("CLASS_NAME", std::move(className)));
    replacements.insert(std::make_pair("PROTOCOL_CLASS_NAME", protClassName()));
    replacements.insert(std::make_pair("BEGIN_NAMESPACE", std::move(namespaces.first)));
    replacements.insert(std::make_pair("END_NAMESPACE", std::move(namespaces.second)));
    replacements.insert(std::make_pair("APPEND", m_generator.getExtraAppendForPluginSrcInPlugin(pluginClassName())));

    std::string str = common::processTemplate(Templ, replacements);

    std::ofstream stream(filePath);
    if (!stream) {
        m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
        return false;
    }
    stream << str;

    if (!stream.good()) {
        m_generator.logger().error("Failed to write \"" + filePath + "\".");
        return false;
    }

    return true;
}

bool Plugin::writePluginJson()
{
    auto filePath = m_generator.startProtocolPluginCommonWrite(pluginClassName(), ".json");

    if (filePath.empty()) {
        return true;
    }

    static const std::string Templ =
        "{\n"
        "    \"name\" : \"#^#NAME#$#\",\n"
        "    \"desc\" : [\n"
        "        #^#DESC#$#\n"
        "    ],\n"
        "    \"type\" : \"protocol\"\n"
        "}\n";

    auto name = adjustedName() + " Protocol";
    auto desc = common::makeMultilineCopy(m_description);
    if (!desc.empty()) {
        desc = '\"' + desc + '\"';
        ba::replace_all(desc, "\n", "\",\n\"");
    }
    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("NAME", std::move(name)));
    replacements.insert(std::make_pair("DESC", std::move(desc)));

    std::string str = common::processTemplate(Templ, replacements);

    std::ofstream stream(filePath);
    if (!stream) {
        m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
        return false;
    }
    stream << str;

    if (!stream.good()) {
        m_generator.logger().error("Failed to write \"" + filePath + "\".");
        return false;
    }

    return true;
}

bool Plugin::writePluginConfig()
{
    auto filePath = m_generator.startProtocolPluginCommonWrite(common::nameToClassCopy(common::updateNameCopy(adjustedName())), ".cfg");

    if (filePath.empty()) {
        return true;
    }

    static const std::string Templ =
        "{\n"
        "    \"cc_plugins_list\": [\n"
        "        \"cc.EchoSocketPlugin\",\n"
        "        \"#^#ID#$#\"\n"
        "    ]\n"
        "}\n";

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("ID", pluginId()));
    std::string str = common::processTemplate(Templ, replacements);

    std::ofstream stream(filePath);
    if (!stream) {
        m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
        return false;
    }
    stream << str;

    if (!stream.good()) {
        m_generator.logger().error("Failed to write \"" + filePath + "\".");
        return false;
    }

    return true;
}

std::string Plugin::protClassName() const
{
    return common::nameToClassCopy(common::updateNameCopy(adjustedName())) + ProtSuffix;
}

std::string Plugin::pluginClassName() const
{
    return common::nameToClassCopy(common::updateNameCopy(adjustedName())) + PluginSuffix;
}

std::string Plugin::pluginId() const
{
    auto id = m_generator.schemaName();
    if (!m_name.empty()) {
        id += '.' + m_name;
    }

    return id;
}

} // namespace commsdsl2comms

