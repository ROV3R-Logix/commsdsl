//
// Copyright 2018 (C). Alex Robenko. All rights reserved.
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

#pragma once

#include "commsdsl/ListField.h"

#include "Field.h"
#include "common.h"

namespace commsdsl2comms
{

class ListField : public Field
{
    using Base = Field;
public:
    ListField(Generator& generator, commsdsl::Field field) : Base(generator, field) {}

protected:
    virtual bool prepareImpl() override;
    virtual void updateIncludesImpl(IncludesList& includes) const override;
    virtual void updatePluginIncludesImpl(IncludesList& includes) const override;
    virtual std::size_t maxLengthImpl() const override;
    virtual std::string getClassDefinitionImpl(
        const std::string& scope,
        const std::string& className) const override;
    virtual std::string getExtraDefaultOptionsImpl(const std::string& scope) const override;
    virtual std::string getCompareToValueImpl(
        const std::string& op,
        const std::string& value,
        const std::string& nameOverride,
        bool forcedVersionOptional) const override;
    virtual std::string getCompareToFieldImpl(
        const std::string& op,
        const Field& field,
        const std::string& nameOverride,
        bool forcedVersionOptional) const override;
    virtual std::string getPluginAnonNamespaceImpl(
        const std::string& scope,
        bool forcedSerialisedHidden,
        bool serHiddenParam) const override;
    virtual std::string getPluginPropertiesImpl(bool serHiddenParam) const override;
    virtual std::string getPrivateRefreshBodyImpl(const FieldsList& fields) const override;
    virtual bool hasCustomReadRefreshImpl() const override;
    virtual std::string getReadPreparationImpl(const FieldsList& fields) const override;
    virtual bool isLimitedCustomizableImpl() const override;

private:
    using StringsList = common::StringsList;

    std::string getFieldOpts(const std::string& scope) const;
    std::string getElement() const;
    std::string getMembersDef(const std::string& scope) const;
    void checkFixedSizeOpt(StringsList& list) const;
    void checkCountPrefixOpt(StringsList& list) const;
    void checkLengthPrefixOpt(StringsList& list) const;
    void checkElemLengthPrefixOpt(StringsList& list) const;
    bool checkDetachedPrefixOpt(StringsList& list) const;
    bool isElemForcedSerialisedHiddenInPlugin() const;
    std::string getPrefixName() const;


    commsdsl::ListField listFieldDslObj() const
    {
        return commsdsl::ListField(dslObj());
    }

    FieldPtr m_element;
    FieldPtr m_countPrefix;
    FieldPtr m_lengthPrefix;
    FieldPtr m_elemLengthPrefix;
};

inline
FieldPtr createListField(Generator& generator, commsdsl::Field field)
{
    return std::make_unique<ListField>(generator, field);
}

} // namespace commsdsl2comms
