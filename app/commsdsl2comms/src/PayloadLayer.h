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

#pragma once

#include "commsdsl/Layer.h"

#include "Layer.h"
#include "common.h"

namespace commsdsl2comms
{

class PayloadLayer : public Layer
{
    using Base = Layer;
public:
    PayloadLayer(Generator& generator, commsdsl::Layer layer) : Base(generator, layer) {}

protected:
    virtual void updateIncludesImpl(IncludesList& includes) const override final;
    virtual std::string getClassDefinitionImpl(
        const std::string& scope,
        std::string& prevLayer,
        bool& hasInputMessages) const override final;
    virtual const std::string& getBareMetalOptionStrImpl() const override final;
    virtual bool isCustomizableImpl() const override final;

private:
    commsdsl::PayloadLayer payloadLayerDslObj() const
    {
        return commsdsl::PayloadLayer(dslObj());
    }

};

inline
LayerPtr createPayloadLayer(Generator& generator, commsdsl::Layer layer)
{
    return std::make_unique<PayloadLayer>(generator, layer);
}

} // namespace commsdsl2comms
