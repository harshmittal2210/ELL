////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     main.cpp (nodes_test)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "AppendNodesTests.h"
#include "NodesTests.h"

#include <testing/include/testing.h>

#include <utilities/include/Exception.h>
#include <utilities/include/Unused.h>

#include <iostream>

using namespace ell;

/// Runs all tests
///
int main(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    try
    {
        TestNodes();
        TestAppendNodes();
    }
    catch (const utilities::Exception& exception)
    {
        std::cerr << "ERROR, got ELL exception. Message: " << exception.GetMessage() << std::endl;
        throw;
    }

    if (testing::DidTestFail())
    {
        return 1;
    }

    return 0;
}
