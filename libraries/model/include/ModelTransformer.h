////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     ModelTransformer.h (model)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "InputPort.h"
#include "Model.h"
#include "Node.h"
#include "OutputPort.h"
#include "Port.h"
#include "PortElements.h"
#include "Submodel.h"

#include <utilities/include/Exception.h>

#include <cassert>
#include <exception>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ell
{
namespace model
{
    class InputNodeBase;
    template <typename ValueType>
    class InputNode;
    class MapCompiler;

    /// <summary> An action to perform on a node during transformation (refinement/compilation) </summary>
    enum class NodeAction
    {
        abstain,
        refine,
        compile
    };

    /// <summary> A function that determines how to process a node </summary>
    using NodeActionFunction = std::function<NodeAction(const Node&)>;

    /// <summary> A context object that carries information about the compiler or other process driving the transformation. </summary>
    class TransformContext
    {
    public:
        /// <summary> Default Constructor. </summary>
        TransformContext();

        /// <summary> Constructor </summary>
        ///
        /// <param name='nodeActionFunction'> A function that indicates how to override the default refinement or compilation of a node </param>
        TransformContext(const NodeActionFunction& nodeActionFunction);

        /// <summary> Constructor </summary>
        ///
        /// <param name='compiler'> The MapCompiler that is currently compiling the model </param>
        /// <param name='nodeActionFunction'> A function that indicates how to override the default refinement or compilation of a node </param>
        TransformContext(const MapCompiler* compiler, const NodeActionFunction& nodeActionFunction);

        /// <summary> Indicates if a node is compilable. </summary>
        ///
        /// <param name="node"> A node. </param>
        /// <returns> Returns true if the node is compilable. </returns>
        bool IsNodeCompilable(const Node& node) const;

        /// <summary> Gets the map compiler. </summary>
        ///
        /// <returns> Returns a pointer to the map compiler (or nullptr if one isn't defined). </returns>
        const MapCompiler* GetCompiler() const { return _compiler; }

        /// <summary> Adds a custom node action function to call during refinement </summary>
        ///
        /// <param name='nodeActionFunction'> A function that indicates how to override the default refinement or compilation of a node </param>
        void AddNodeActionFunction(const NodeActionFunction& nodeActionFunction);

        /// <summary>
        /// Gets the action to take on the node during refinement. If any custom node action
        /// have been registered with this context, return the result of the last one that
        /// returns something other than `NodeAction::abstain`. If all of the functions
        /// abstain, or there are no custom functions, return `NodeAction::compile` if the node
        /// is compilable, otherwise return `NodeAction::refine`.
        /// </summary>
        ///
        /// <param name="node"> A node. </param>
        /// <returns> A `NodeAction` enum indicating what action to take on the node </returns>
        NodeAction GetNodeAction(const Node& node) const;

    private:
        std::vector<NodeActionFunction> _nodeActionFunctions;
        const MapCompiler* _compiler;
    };

    /// <summary> A class that transforms models (including refinement and copying) </summary>
    class ModelTransformer
    {
    public:
        using NodeTransformFunction = std::function<void(const Node&, ModelTransformer&)>;

        /// <summary> Returns a copy of the input model, by calling Copy() on each of the model's nodes </summary>
        ///
        /// <param name="model"> The model. </param>
        ///
        /// <returns> A copy of the input model. </returns>
        Model CopyModel(const Model& model);

        /// <summary> Returns a copy of the input model, by calling Copy() on each of the model's nodes </summary>
        ///
        /// <param name="model"> The model. </param>
        /// <param name="context"> The context. </param>
        ///
        /// <returns> A copy of the input model. </returns>
        Model CopyModel(const Model& model, const TransformContext& context);

        /// <summary>
        /// Returns a new model containing a copy of a submodel, by calling Copy() on each of the submodel's nodes.
        /// </summary>
        ///
        /// <param name="submodel"> The submodel to copy. There must not be any free input ports in the submodel. </param>
        /// <param name="context"> The context. </param>
        ///
        /// <returns> A copy of the requested portion of the model. </returns>
        Submodel CopySubmodel(const Submodel& submodel, const TransformContext& context);

        /// <summary>
        /// Copies part of a source model onto a specified place in a destination model. If the source
        /// and destination models are the same model, then the copy is done in-place: nodes are copied
        /// in-place within the model.
        /// </summary>
        ///
        /// A submodel is specified by the outputs it computes and the inputs that will be grafted onto
        /// new locations in the destination model. For a given output (or set of outputs), the choice of
        /// inputs determines the size of the suffix to copy. For instance, the simple linear model
        /// `A -> B -> C -> D`, includes the following submodels:
        ///
        /// A
        /// A->B
        /// A->B->C
        /// A->B->C->D
        /// B
        /// B->C
        /// B->C->D
        /// C
        /// C->D
        /// D
        ///
        /// If D is specified as the output of the submodel, any of A, B, C, or D would be reasonable inputs.
        ///
        /// When performing an in-place copy, trivial node copies are elided: if a node is to be copied to
        /// its existing location (that is, the new copy will havethe same inputs as the original node),
        /// then no copy is performed. For instance, `ConstantNode`s will always have their copies elided.
        ///
        /// Consider the the following model:
        ///
        /// ```
        ///         X--v
        ///  I -> B -> + -> C -> out1
        ///   \-> B' -> out2
        /// ```
        ///
        /// If we wanted to copy the submodel representing the output of `B` up to the output of `C` onto the output of B',
        /// we would create a new copy of the `+` and `C` nodes, but would use the same constant data (the `X` node).
        ///
        ///
        /// <param name="submodel"> The submodel to copy. There must not be any free input ports in the submodel. </param>
        /// <param name="destModel"> The model to append transformed nodes into. </param>
        /// <param name="onto"> 
        ///     The output ports in the destination model to graft the copied submodel's inputs onto. 
        ///     The "onto" ports must be in the same order as the corresponding inputs in the source submodel.
        ///  </param>
        /// <param name="context"> The context. </param>
        ///
        /// <returns> A copy of the requested portion of the model. </returns>
        Submodel CopySubmodelOnto(const Submodel& submodel, Model& destModel, const std::vector<const OutputPortBase*>& onto, const TransformContext& context);

        /// <summary>
        /// Performs one or more refinement iterations on a given model and returns the result.
        /// If `context.IsNodeCompilable()` is false, this call performs one refinement iteration. If
        /// `context.IsNodeCompilable()` is true, this call refines the model until all its nodes are
        /// compilable or until none of the nodes refine themselves.
        /// </summary>
        ///
        /// <param name="model"> The model. </param>
        /// <param name="context"> The context. </param>
        /// <param name="maxIterations"> The maximum number of refinement iterations to perform. </param>
        ///
        /// <returns> The refined Model. </returns>
        Model RefineModel(const Model& model, const TransformContext& context, int maxIterations = 10);

        /// <summary> Transforms the model by applying a transformation function to each node </summary>
        ///
        /// <param name="model"> The model to transform. </param>
        /// <param name="context"> The TransformContext to use during the transformation </param>
        /// <param name="transformFunction"> The function to apply on each node </param>
        ///
        /// <returns> The transformed Model. </returns>
        Model TransformModel(const Model& model,
                             const TransformContext& context,
                             const NodeTransformFunction& transformFunction);

        /// <summary> Transforms part of a model by applying a transformation function to each node, putting the result into a given place in (potentially the same) model </summary>
        ///
        /// <param name="submodel"> The submodel to transform. </param>
        /// <param name="destModel"> The model to append transformed nodes into. </param>
        /// <param name="onto"> 
        ///     The output ports in the destination model to graft the transformed submodel's inputs onto. 
        ///     The "onto" ports must be in the same order as the corresponding inputs in the source submodel.
        ///  </param>
        /// <param name="context"> The TransformContext to use during the transformation. </param>
        /// <param name="transformFunction"> The function to apply to each node. </param>
        ///
        /// <returns> The resulting (transformed) submodel. </returns>
        Submodel TransformSubmodelOnto(const Submodel& submodel,
                                       Model& destModel,
                                       const std::vector<const OutputPortBase*>& onto,
                                       const TransformContext& context,
                                       const NodeTransformFunction& transformFunction);

        /// <summary> Transforms part of a model in-place by applying a transformation function to each node, putting the result into a given place in the model. </summary>
        ///
        /// <param name="submodel"> The submodel to transform. </param>
        /// <param name="onto"> 
        ///     The output ports in the destination model to graft the transformed submodel's inputs onto. 
        ///     The "onto" ports must be in the same order as the corresponding inputs in the source submodel.
        ///  </param>
        /// <param name="context"> The TransformContext to use during the transformation. </param>
        /// <param name="transformFunction"> The function to apply to each node. </param>
        ///
        /// <returns> The resulting (transformed) submodel. </returns>
        Submodel TransformSubmodelOnto(const Submodel& submodel,
                                       const std::vector<const OutputPortBase*>& onto,
                                       const TransformContext& context,
                                       const NodeTransformFunction& transformFunction);

        /// <summary> Resets the internal state of the transformer </summary>
        void Reset();

        // for debugging
        bool IsEmpty() const { return _elementsMap.IsEmpty(); }

        /// <summary> Returns the port from the new model corresponding to the given input port on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        template <typename ValueType>
        const OutputPort<ValueType>& GetCorrespondingInputs(const InputPort<ValueType>& port) const;

        /// <summary> Returns the port from the new model corresponding to the given input port on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        const OutputPortBase& GetCorrespondingInputs(const InputPortBase& port) const;

        /// <summary> Returns the port from the new model corresponding to the given port on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        template <typename ValueType>
        const OutputPort<ValueType>& GetCorrespondingOutputs(const OutputPort<ValueType>& port) const;

        /// <summary> Returns the port from the new model corresponding to the given port on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        const OutputPortBase& GetCorrespondingOutputs(const OutputPortBase& port) const;

        /// <summary> Returns the output port from the new model corresponding to the given input port on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        template <typename ValueType>
        const OutputPort<ValueType>& GetCorrespondingOutputs(const InputPort<ValueType>& port) const;

        /// <summary> Returns the output port from the new model corresponding to the given input port on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        const OutputPortBase& GetCorrespondingOutputs(const InputPortBase& port) const;

        /// <summary> Returns the ports from the new model corresponding to the given ports on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        std::vector<const OutputPortBase*> GetCorrespondingOutputs(const std::vector<const OutputPortBase*>& ports) const;

        /// <summary> Returns the ports from the new model corresponding to the given elements on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        template <typename ValueType>
        const OutputPort<ValueType>& GetCorrespondingOutputs(const PortElements<ValueType>& elements) const;

        /// <summary> Returns the ports from the new model corresponding to the given elements on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        const OutputPortBase& GetCorrespondingOutputs(const PortElementsBase& elements) const;

        /// <summary> Returns the input node from the new model corresponding to the given input node on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        template <typename ValueType>
        InputNode<ValueType>* GetCorrespondingInputNode(const InputNode<ValueType>* node) const;

        /// <summary> Returns the input node from the new model corresponding to the given input node on the input model </summary>
        /// <remarks> Only available after calling CopyModel or RefineModel </remarks>
        InputNodeBase* GetCorrespondingInputNode(const InputNodeBase* node) const;

        ///
        /// Functions used by node implementors
        ///

        /// <summary> Creates a new node in the transformed model. Called by node implementors. </summary>
        ///
        /// <typeparam name="Args"> The arguments to the constructor of NodeType. </typeparam>
        template <typename NodeType, typename... Args>
        NodeType* AddNode(Args&&... args);

        /// <summary> Adds nodes to the transformed model to represent "complex" outputs: the concatenation of arbitrary subsets of output ports </summary>
        ///
        /// <param name="elements"> The output port elements to route to a new output port. </param>
        ///
        /// <returns> An output port containing the values specified by the `elements` argument. </returns>
        template <typename ValueType>
        const OutputPort<ValueType>& SimplifyOutputs(const PortElements<ValueType>& elements);

        /// <summary> Adds nodes to the transformed model to represent "complex" outputs: the concatenation of arbitrary subsets of output ports </summary>
        ///
        /// <param name="elements"> The output port elements to route to a new output port. </param>
        ///
        /// <returns> An output port containing the values specified by the `elements` argument. </returns>
        const OutputPortBase& SimplifyOutputs(const PortElementsBase& elements);

        /// <summary> Deletes the target node in the new model </summary>
        ///
        /// <param name="node"> The target node to delete in the new model </param>
        /// <remarks> This is only safe to call before any action has been taken on this node (such as copying) </remarks>
        void DeleteNode(const Node& node);

        /// <summary> Copies the target node in the new model </summary>
        ///
        /// <param name="node"> The target node to copy in the new model </param>
        void CopyNode(const Node& node);

        /// <summary> Refines the target node in the new model </summary>
        ///
        /// <param name="node"> The target node to refine in the new model </param>
        ///
        /// <returns> Returns `true` if the node refined itself into something different. </returns>
        bool RefineNode(const Node& node);

        /// <summary> Sets up an old-to-new model output mapping. Called by node implementors </summary>
        ///
        /// <param name="oldPort"> The port in the old model to map to the new model. </param>
        /// <param name="newPort"> The port in the new model to be mapped from the old model. </param>
        template <typename ValueType>
        void MapNodeOutput(const OutputPort<ValueType>& oldPort, const OutputPortBase& newPort);

        /// <summary> Sets up an old-to-new model output mapping. Called by node implementors </summary>
        ///
        /// <param name="oldPort"> The port in the old model to map to the new model. </param>
        /// <param name="newPort"> The port in the new model to be mapped from the old model. </param>
        template <typename ValueType>
        void MapNodeOutput(const OutputPort<ValueType>& oldPort, const OutputPort<ValueType>& newPort);

        /// <summary> Get the context used by the transformer. Called by node implementors </summary>
        ///
        /// <returns> The context in use by the transformer. </returns>
        TransformContext& GetContext() { return _context; }

        /// <summary> Get the context used by the transformer. Called by node implementors </summary>
        ///
        /// <returns> The context in use by the transformer. </returns>
        const TransformContext& GetContext() const { return _context; }

        Model& GetModel() { return _model; }

    private:
        friend class Node;

        class PortOutputsMap
        {
        public:
            void Clear();
            bool IsEmpty() const;
            bool IsOutputMapped(const OutputPortBase& queryPort) const;
            const OutputPortBase& GetCorrespondingPort(const OutputPortBase& port) const;
            void MapNodeOutput(const OutputPortBase* oldPort, const OutputPortBase* newPort);
            static PortOutputsMap ConcatenateMaps(const PortOutputsMap& oldMap, const PortOutputsMap& newMap);

        private:
            std::unordered_map<const OutputPortBase*, const OutputPortBase*> _outputPortMap;
        };

        void MapNodeOutput(const OutputPortBase& oldPort, const OutputPortBase& newPort);
        bool ShouldCopyNode(const Node& node) const;
        bool IsInputMapped(const InputPortBase& input) const;
        bool IsOutputMapped(const OutputPortBase& output) const;
        bool IsInputNode(const Node& node) const;
        static bool Compatible(const InputPortBase* source, const OutputPortBase* dest);
        void MapCorrespondingInputs(const std::vector<const InputPortBase*>& sources, const std::vector<const OutputPortBase*>& destinations);
        bool IsInPlace() const;

        template <typename NodeType>
        NodeType* GetCorrespondingInputNodeAs(const NodeType* node) const;

        void ResetContext();
        std::vector<const Node*> FindUncompilableNodes(const Model& model, const TransformContext& context) const;

        /// <summary>
        /// Assign ancestor to newly transformed or refined nodes. This maps relationship between nodes of original
        /// model and nodes of new model. Note that, this assumes new nodes are always appended at the end of existing
        /// nodes. Thus, it assigns ancestor from the node at the end of model to the last node without ancestor.
        /// </summary>
        ///
        /// <param name="ancestorNode"> The ancestor node or the immediate parent node that contains ancestor information. </param>
        void AssignNodeAncestor(const Node& ancestorNode);

        Model _model;
        TransformContext _context;
        PortOutputsMap _elementsMap;
        bool _isModelCompilable = false;
        bool _isInPlace = false;
    };
} // namespace model
} // namespace ell

#pragma region implementation

namespace ell
{
namespace model
{
    //
    // ModelTransformer
    //
    template <typename ValueType>
    const OutputPort<ValueType>& ModelTransformer::GetCorrespondingInputs(const InputPort<ValueType>& port) const
    {
        return GetCorrespondingOutputs(port);
    }

    template <typename ValueType>
    const OutputPort<ValueType>& ModelTransformer::GetCorrespondingOutputs(const OutputPort<ValueType>& port) const
    {
        const auto& result = GetCorrespondingOutputs(static_cast<const OutputPortBase&>(port));
        return static_cast<const OutputPort<ValueType>&>(result);
    }

    template <typename ValueType>
    const OutputPort<ValueType>& ModelTransformer::GetCorrespondingOutputs(const InputPort<ValueType>& port) const
    {
        const auto& result = GetCorrespondingInputs(static_cast<const InputPortBase&>(port));
        return static_cast<const OutputPort<ValueType>&>(result);
    }

    template <typename ValueType>
    const OutputPort<ValueType>& ModelTransformer::GetCorrespondingOutputs(const PortElements<ValueType>& elements) const
    {
        if (!elements.IsFullPortOutput())
        {
            throw utilities::InputException(utilities::InputExceptionErrors::invalidArgument, "ModelTransformer::GetCorrespondingOutputs(): Invalid PortElements");
        }
        const auto& result = GetCorrespondingOutputs(*elements.GetRanges()[0].ReferencedPort());
        return static_cast<const OutputPort<ValueType>&>(result);
    }

    template <typename NodeType>
    NodeType* ModelTransformer::GetCorrespondingInputNodeAs(const NodeType* inputNode) const
    {
        const auto& newNodeOutputs = GetCorrespondingOutputs(inputNode->GetOutputPort());
        auto newNodeConst = newNodeOutputs.GetNode();
        auto newInputNodeConst = dynamic_cast<const NodeType*>(newNodeConst);
        assert(newInputNodeConst != nullptr);
        auto newInputNode = const_cast<NodeType*>(newInputNodeConst);
        return newInputNode;
    }

    template <typename ValueType>
    InputNode<ValueType>* ModelTransformer::GetCorrespondingInputNode(const InputNode<ValueType>* inputNode) const
    {
        return GetCorrespondingInputNodeAs(inputNode);
    }

    template <typename NodeType, typename... Args>
    NodeType* ModelTransformer::AddNode(Args&&... args)
    {
        auto newNode = _model.AddNode<NodeType>(std::forward<Args>(args)...);
        _isModelCompilable &= _context.IsNodeCompilable(*newNode);
        return newNode;
    }

    template <typename ValueType>
    const OutputPort<ValueType>& ModelTransformer::SimplifyOutputs(const PortElements<ValueType>& elements)
    {
        return _model.SimplifyOutputs(elements);
    }

    template <typename ValueType>
    void ModelTransformer::MapNodeOutput(const OutputPort<ValueType>& oldPort, const OutputPortBase& newPort)
    {
        _elementsMap.MapNodeOutput(&oldPort, &newPort);
    }

    template <typename ValueType>
    void ModelTransformer::MapNodeOutput(const OutputPort<ValueType>& oldPort, const OutputPort<ValueType>& newPort)
    {
        _elementsMap.MapNodeOutput(&oldPort, &newPort);
    }
} // namespace model
} // namespace ell

#pragma endregion implementation
