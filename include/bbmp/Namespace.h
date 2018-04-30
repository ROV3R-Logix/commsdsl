#pragma once

#include <string>
#include <vector>

#include "BbmpApi.h"
#include "Field.h"
#include "Message.h"

namespace bbmp
{

class NamespaceImpl;
class BBMP_API Namespace
{
public:
    using NamespacesList = std::vector<Namespace>;
    using FieldsList = std::vector<Field>;
    using MessagesList = std::vector<Message>;

    explicit Namespace(const NamespaceImpl* impl);
    Namespace(const Namespace& other);
    ~Namespace();

    const std::string& name() const;
    const NamespacesList& namespaces() const;
    FieldsList fields() const;
    MessagesList messages() const;

private:
    const NamespaceImpl* m_pImpl;
};

} // namespace bbmp
