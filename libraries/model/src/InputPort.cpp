////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     InputPort.cpp (model)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "InputPort.h"

namespace ell
{
namespace model
{
    //
    // InputPortBase
    //
    InputPortBase::InputPortBase(Port::PortType portType) :
        Port(nullptr, "", portType)
    {}

    InputPortBase::InputPortBase(const Node* owningNode, const OutputPortBase& input, const std::string& name) :
        Port(owningNode, name, input.GetType())
    {
        SetReferencedPort(&input);
    }

    InputPortBase::~InputPortBase()
    {
        if (_referencedPort)
        {
            _referencedPort->RemoveReference(this);
        }
    }

    std::vector<const Node*> InputPortBase::GetParentNodes() const
    {
        return _referencedPort == nullptr ? std::vector<const Node*>()
                                          : std::vector<const Node*>{ _referencedPort->GetNode() };
    }

    PortElementBase InputPortBase::GetInputElement(size_t index) const
    {
        if (!IsValid())
        {
            throw utilities::LogicException(utilities::LogicExceptionErrors::illegalState, "Error: empty input port.");
        }

        PortElementsBase elements(GetReferencedPort());
        return elements.GetElement(index);
    }

    PortMemoryLayout InputPortBase::GetMemoryLayout() const
    {
        return _referencedPort->GetMemoryLayout();
    }

    const OutputPortBase& InputPortBase::GetReferencedPort() const
    {
        if (!IsValid())
        {
            throw utilities::LogicException(utilities::LogicExceptionErrors::illegalState, "Error: empty input port.");
        }

        return *_referencedPort;
    }

    size_t InputPortBase::Size() const
    {
        if (!IsValid())
        {
            return 0;
        }
        return GetReferencedPort().Size();
    }

    bool InputPortBase::IsValid() const
    {
        return _referencedPort != nullptr;
    }

    void InputPortBase::SetReferencedPort(const OutputPortBase* input)
    {
        if (_referencedPort)
        {
            _referencedPort->RemoveReference(this);
        }
        if (input)
        {
            input->AddReference(this);
        }
        _referencedPort = input;
    }

    void InputPortBase::ClearReferencedPort()
    {
        _referencedPort = nullptr;
    }
    
    void InputPortBase::UpdateReferencedPort()
    {
        if (!_referencedPort->HasReference(this))
        {
            _referencedPort->AddReference(this);
        }
    }
} // namespace model
} // namespace ell
