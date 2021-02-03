//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "ASUtils.h"
#include "Tuning.h"
#include "Utils.h"
#include "XmlAnalyzer.h"
#include "XmlSourceData.h"

#include <cassert>
#include <regex>

namespace ASBindingGenerator
{

// https://www.angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_primitives.html
// https://en.cppreference.com/w/cpp/language/types
string CppFundamentalTypeToAS(const string& cppType)
{
    // AngelScript itself detect width of bool type (look AS_SIZEOF_BOOL define)
    if (cppType == "bool")
        return "bool";

    if (cppType == "char" || cppType == "signed char")
        return "int8";

    if (cppType == "unsigned char")
        return "uint8";

    if (cppType == "short")
        return "int16";

    if (cppType == "unsigned short")
        return "uint16";

    if (cppType == "int")
        return "int";

    if (cppType == "unsigned" || cppType == "unsigned int")
        return "uint";

    if (cppType == "long long")
        return "int64";

    if (cppType == "unsigned long long")
        return "uint64";

    if (cppType == "float")
        return "float";

    if (cppType == "double")
        return "double";

    // Types below have different width on different systems and are registered in Manual.cpp
    if (cppType == "long")
        return "long";

    if (cppType == "unsigned long")
        return "ulong";

    if (cppType == "size_t")
        return "size_t";

    throw Exception(cppType + " not a fundamental type");
}

shared_ptr<EnumAnalyzer> FindEnum(const string& name)
{
    NamespaceAnalyzer namespaceAnalyzer(SourceData::namespaceUrho3D_);
    vector<EnumAnalyzer> enumAnalyzers = namespaceAnalyzer.GetEnums();

    for (const EnumAnalyzer& enumAnalyzer : enumAnalyzers)
    {
        if (enumAnalyzer.GetTypeName() == name)
            return make_shared<EnumAnalyzer>(enumAnalyzer);
    }

    return shared_ptr<EnumAnalyzer>();
}

static bool IsUsing(const string& identifier)
{
    for (xml_node memberdef : SourceData::usings_)
    {
        UsingAnalyzer usingAnalyzer(memberdef);
        
        if (usingAnalyzer.GetName() == identifier)
            return true;
    }

    return false;
}

bool IsKnownCppType(const string& name)
{
    static vector<string> _knownTypes = {
        "void",
        "bool",
        "size_t",
        "char",
        "signed char",
        "unsigned char",
        "short",
        "unsigned short",
        "int",
        "long",
        "unsigned",
        "unsigned int",
        "unsigned long",
        "long long",
        "unsigned long long",
        "float",
        "double",
        "SDL_JoystickID",

        // TODO: Remove
        "VariantMap",
    };

    if (CONTAINS(_knownTypes, name))
        return true;

    if (SourceData::classesByName_.find(name) != SourceData::classesByName_.end())
        return true;

    if (SourceData::enums_.find(name) != SourceData::enums_.end())
        return true;

    if (EndsWith(name, "Flags"))
        return true;

    return false;
}

shared_ptr<ClassAnalyzer> FindClassByName(const string& name)
{
    auto it = SourceData::classesByName_.find(name);
    if (it != SourceData::classesByName_.end())
    {
        xml_node compounddef = it->second;
        return make_shared<ClassAnalyzer>(compounddef);
    }

    // using VariantVector = Vector<Variant>

    return shared_ptr<ClassAnalyzer>();
}

shared_ptr<ClassAnalyzer> FindClassByID(const string& id)
{
    auto it = SourceData::classesByID_.find(id);
    if (it != SourceData::classesByID_.end())
    {
        xml_node compounddef = it->second;
        return make_shared<ClassAnalyzer>(compounddef);
    }

    // using VariantVector = Vector<Variant>

    return shared_ptr<ClassAnalyzer>();
}

string CppTypeToAS(const TypeAnalyzer& type, bool returnType)
{
    if (type.IsRvalueReference() || type.IsDoublePointer() || type.IsRefToPointer())
        throw Exception("Error: type \"" + type.ToString() + "\" can not automatically bind");

    string cppTypeName = type.GetNameWithTemplateParams();

    if (cppTypeName == "Context" && returnType)
        throw Exception("Error: type \"" + type.ToString() + "\" can not be returned");

    if (!IsKnownCppType(type.GetNameWithTemplateParams()))
        throw Exception("Error: type \"" + type.ToString() + "\" can not automatically bind");

    shared_ptr<ClassAnalyzer> analyzer = FindClassByName(type.GetNameWithTemplateParams());
    if (analyzer && analyzer->IsInternal())
        throw Exception("Error: type \"" + type.ToString() + "\" can not automatically bind bacause internal");

    if (analyzer && Contains(analyzer->GetComment(), "NO_BIND"))
        throw Exception("Error: type \"" + cppTypeName + "\" can not automatically bind bacause have @nobind mark");

    // analyzer can be null for simple types (int, float) or if type "using VariantVector = Vector<Variant>"
    // TODO add to type info "IsUsing"
    // TODO add description to TypeAnalyzer::GetClass()

    if (IsUsing(cppTypeName) && cppTypeName != "VariantMap")
        throw Exception("Using \"" + cppTypeName + "\" can not automatically bind");

    string asTypeName;
    
    try
    {
        asTypeName = CppFundamentalTypeToAS(cppTypeName);
    }
    catch (...)
    {
        asTypeName = cppTypeName;
    }

    if (asTypeName == "void" && type.IsPointer())
        throw Exception("Error: type \"void*\" can not automatically bind");

    if (asTypeName.find('<') != string::npos)
        throw Exception("Error: type \"" + type.ToString() + "\" can not automatically bind");

    if (Contains(type.ToString(), "::"))
        throw Exception("Error: type \"" + type.ToString() + "\" can not automatically bind bacause internal");

    if (type.IsConst() && type.IsReference() && !returnType)
        return "const " + asTypeName + "&in";

    string result = asTypeName;

    if (type.IsReference())
    {
        result += "&";
    }
    else if (type.IsPointer())
    {
        shared_ptr<ClassAnalyzer> analyzer = FindClassByName(type.GetNameWithTemplateParams());

        if (analyzer && (analyzer->IsRefCounted() || Contains(analyzer->GetComment(), "FAKE_REF")))
            result += "@+";
        else
            throw Exception("Error: type \"" + type.ToString() + "\" can not automatically bind");
    }

    if (returnType && type.IsConst() && !type.IsPointer())
        result = "const " + result;

    return result;
}

string CppValueToAS(const string& cppValue)
{
    if (cppValue == "nullptr")
        return "null";

    if (cppValue == "Variant::emptyVariantMap")
        return "VariantMap()";

    if (cppValue == "NPOS")
        return "String::NPOS";

    return cppValue;
}

// =================================================================================

shared_ptr<FuncParamConv> CppFunctionParamToAS(const ParamAnalyzer& paramAnalyzer)
{
    shared_ptr<FuncParamConv> result = make_shared<FuncParamConv>();

    TypeAnalyzer typeAnalyzer = paramAnalyzer.GetType();
    string cppTypeName = typeAnalyzer.GetNameWithTemplateParams();

    if (cppTypeName == "Context")
    {
        result->errorMessage_ = "Context can be used as firs parameter of constructors only";
        return result;
    }

    if (cppTypeName == "Vector<String>" && typeAnalyzer.IsConst() && typeAnalyzer.IsReference())
    {
        result->success_ = true;
        result->inputVarName_ = paramAnalyzer.GetDeclname();
        result->convertedVarName_ = result->inputVarName_ + "_conv";
        result->glue_ = "    Vector<String> " + result->convertedVarName_ + " = ArrayToVector<String>(" + result->inputVarName_ + ");\n";
        result->cppType_ = "CScriptArray*";
        //result->asDecl_ = "String[]&";
        result->asDecl_ = "Array<String>@+";

        string defval = paramAnalyzer.GetDefval();
        if (!defval.empty())
        {
            assert(defval == "Vector< String >()");
            //result->asDecl_ += " = Array<String>()";
            result->asDecl_ += " = null";
        }

        return result;
    }

    smatch match;
    regex_match(cppTypeName, match, regex("PODVector<(\\w+)>"));
    if (match.size() == 2 && typeAnalyzer.IsConst() && typeAnalyzer.IsReference())
    {
        string cppTypeName = match[1].str();
        
        string asTypeName;
        
        try
        {
            asTypeName = CppFundamentalTypeToAS(cppTypeName);
        }
        catch (...)
        {
            asTypeName = cppTypeName;
        }

        result->success_ = true;
        result->inputVarName_ = paramAnalyzer.GetDeclname();
        result->convertedVarName_ = result->inputVarName_ + "_conv";
        result->glue_ = "    PODVector<" + cppTypeName + "> " + result->convertedVarName_ + " = ArrayToPODVector<" + cppTypeName + ">(" + result->inputVarName_ + ");\n";
        result->cppType_ = "CScriptArray*";
        result->asDecl_ = "Array<" + asTypeName + ">@+";

        string defval = paramAnalyzer.GetDefval();
        assert(defval.empty()); // TODO: make

        return result;
    }

    regex_match(cppTypeName, match, regex("PODVector<(\\w+)\\*>"));
    // TODO check \\w is refcounted
    if (match.size() == 2 && typeAnalyzer.IsConst() && typeAnalyzer.IsReference())
    {
        string cppTypeName = match[1].str();
        
        string asTypeName;

        try
        {
            asTypeName = CppFundamentalTypeToAS(cppTypeName);
        }
        catch (...)
        {
            asTypeName = cppTypeName;
        }

        result->success_ = true;
        result->inputVarName_ = paramAnalyzer.GetDeclname();
        result->convertedVarName_ = result->inputVarName_ + "_conv";
        result->glue_ = "    PODVector<" + cppTypeName + "*> " + result->convertedVarName_ + " = ArrayToPODVector<" + cppTypeName + "*>(" + result->inputVarName_ + ");\n";
        result->cppType_ = "CScriptArray*";
        result->asDecl_ = "Array<" + asTypeName + "@>@";

        string defval = paramAnalyzer.GetDefval();
        assert(defval.empty()); // TODO: make

        return result;
    }

    regex_match(cppTypeName, match, regex("Vector<SharedPtr<(\\w+)>>"));
    if (match.size() == 2 && typeAnalyzer.IsConst() && typeAnalyzer.IsReference())
    {
        string cppTypeName = match[1].str();
        
        string asTypeName;

        try
        {
            asTypeName = CppFundamentalTypeToAS(cppTypeName);
        }
        catch (...)
        {
            asTypeName = cppTypeName;
        }

        if (cppTypeName == "WorkItem") // TODO autodetect
        {
            result->errorMessage_ = "TODO";
            return result;
        }

        result->success_ = true;
        result->inputVarName_ = paramAnalyzer.GetDeclname();
        result->convertedVarName_ = result->inputVarName_ + "_conv";
        result->glue_ = "    Vector<SharedPtr<" + cppTypeName + "> > " + result->convertedVarName_ + " = HandleArrayToVector<" + cppTypeName + ">(" + result->inputVarName_ + ");\n";
        result->cppType_ = "CScriptArray*";
        result->asDecl_ = "Array<" + asTypeName + "@>@+";

        string defval = paramAnalyzer.GetDefval();
        assert(defval.empty()); // TODO: make

        return result;
    }

    try
    {
        string asType = CppTypeToAS(typeAnalyzer, false);
        result->asDecl_ = asType;
    }
    catch (const Exception& e)
    {
        result->errorMessage_ = e.what();
        return result;
    }

    string defval = paramAnalyzer.GetDefval();
    if (!defval.empty())
    {
        defval = CppValueToAS(defval);
        defval = ReplaceAll(defval, "\"", "\\\"");
        result->asDecl_ += " = " + defval;
    }

    result->success_ = true;
    result->cppType_ = typeAnalyzer.ToString();
    result->inputVarName_ = paramAnalyzer.GetDeclname();
    result->convertedVarName_ = result->inputVarName_;
    return result;
}

shared_ptr<FuncReturnTypeConv> CppFunctionReturnTypeToAS(const TypeAnalyzer& typeAnalyzer)
{
    shared_ptr<FuncReturnTypeConv> result = make_shared<FuncReturnTypeConv>();

    string cppTypeName = typeAnalyzer.GetNameWithTemplateParams();

    if (cppTypeName == "void" && !typeAnalyzer.IsPointer())
    {
        result->success_ = true;
        result->asReturnType_ = "void";
        result->glueReturnType_ = "void";
        result->glueReturn_ = "";
        return result;
    }
    
    if (cppTypeName == "Context")
    {
        result->errorMessage_ = "Error: type \"" + typeAnalyzer.ToString() + "\" can not be returned";
        return result;
    }

    // Works with both Vector<String> and Vector<String>&
    if ((cppTypeName == "Vector<String>" || cppTypeName == "StringVector") && !typeAnalyzer.IsPointer())
    {
        result->success_ = true;
        result->needWrapper_ = true;
        result->asReturnType_ = "Array<String>@";
        result->glueReturnType_ = "CScriptArray*";
        result->glueReturn_ = "return VectorToArray<String>(result, \"Array<String>\");\n";
        return result;
    }

    smatch match;
    regex_match(cppTypeName, match, regex("SharedPtr<(\\w+)>"));
    if (match.size() == 2)
    {
        string cppTypeName = match[1].str();
        
        string asTypeName;

        try
        {
            asTypeName = CppFundamentalTypeToAS(cppTypeName);
        }
        catch (...)
        {
            asTypeName = cppTypeName;
        }

        if (cppTypeName == "WorkItem") // TODO autodetect
        {
            result->errorMessage_ = "TODO";
            return result;
        }

        result->success_ = true;
        result->needWrapper_ = true;
        result->asReturnType_ = asTypeName + "@+";
        result->glueReturnType_ = cppTypeName + "*";
        result->glueReturn_ = "return result.Detach();\n";
        return result;
    }

    regex_match(cppTypeName, match, regex("Vector<SharedPtr<(\\w+)>>"));
    if (match.size() == 2)
    {
        string cppTypeName = match[1].str();
        
        string asTypeName;

        try
        {
            asTypeName = CppFundamentalTypeToAS(cppTypeName);
        }
        catch (...)
        {
            asTypeName = cppTypeName;
        }

        result->success_ = true;
        result->needWrapper_ = true;
        result->asReturnType_ = "Array<" + asTypeName + "@>@";
        result->glueReturnType_ = "CScriptArray*";

        // Which variant is correct/better?
#if 0
        result->glueResult_ = "return VectorToArray<SharedPtr<" + cppTypeName + "> >(result, \"Array<" + cppTypeName + "@>@\");\n";
#else
        result->glueReturn_ = "return VectorToHandleArray(result, \"Array<" + cppTypeName + "@>\");\n";
#endif
        return result;
    }

    regex_match(cppTypeName, match, regex("PODVector<(\\w+)\\*>"));
    if (match.size() == 2)
    {
        string cppTypeName = match[1].str();
        
        string asTypeName;

        try
        {
            asTypeName = CppFundamentalTypeToAS(cppTypeName);
        }
        catch (...)
        {
            asTypeName = cppTypeName;
        }

        result->success_ = true;
        result->needWrapper_ = true;
        result->asReturnType_ = "Array<" + asTypeName + "@>@";
        result->glueReturnType_ = "CScriptArray*";
        result->glueReturn_ = "return VectorToHandleArray(result, \"Array<" + cppTypeName + "@>\");\n";
        return result;
    }

    regex_match(cppTypeName, match, regex("PODVector<(\\w+)>"));
    if (match.size() == 2 && (typeAnalyzer.IsConst() == typeAnalyzer.IsReference()))
    {
        string cppTypeName = match[1].str();
        
        string asTypeName;

        try
        {
            asTypeName = CppFundamentalTypeToAS(cppTypeName);
        }
        catch (...)
        {
            asTypeName = cppTypeName;
        }

        result->success_ = true;
        result->needWrapper_ = true;
        result->asReturnType_ = "Array<" + asTypeName + ">@";
        result->glueReturnType_ = "CScriptArray*";
        result->glueReturn_ = "return VectorToArray(result, \"Array<" + asTypeName + ">\");\n";
        return result;
    }

    try
    {
        string asType = CppTypeToAS(typeAnalyzer, true);
        result->asReturnType_ = asType;
    }
    catch (const Exception& e)
    {
        result->errorMessage_ = e.what();
        return result;
    }

    result->success_ = true;
    result->glueReturn_ = "return result;\n";
    result->glueReturnType_ = typeAnalyzer.ToString();
    return result;
}

// =================================================================================

static string GenerateFunctionWrapperName(xml_node memberdef)
{
    string result = ExtractName(memberdef);

    vector<ParamAnalyzer> params = ExtractParams(memberdef);

    if (params.size() == 0)
    {
        result += "_void";
    }
    else
    {
        for (ParamAnalyzer param : params)
        {
            string t = param.GetType().GetNameWithTemplateParams();
            t = ReplaceAll(t, " ", "");
            t = ReplaceAll(t, "::", "");
            t = ReplaceAll(t, "<", "");
            t = ReplaceAll(t, ">", "");
            t = ReplaceAll(t, "*", "");
            result += "_" + t;
        }
    }

    return result;
}

string GenerateWrapperName(const GlobalFunctionAnalyzer& functionAnalyzer)
{
    return GenerateFunctionWrapperName(functionAnalyzer.GetMemberdef());
}

string GenerateWrapperName(const ClassStaticFunctionAnalyzer& functionAnalyzer)
{
    return functionAnalyzer.GetClassName() + "_" + GenerateFunctionWrapperName(functionAnalyzer.GetMemberdef());
}

string GenerateWrapperName(const ClassFunctionAnalyzer& functionAnalyzer, bool templateVersion)
{
    if (templateVersion)
        return functionAnalyzer.GetClassName() + "_" + GenerateFunctionWrapperName(functionAnalyzer.GetMemberdef()) + "_template";
    else
        return functionAnalyzer.GetClassName() + "_" + GenerateFunctionWrapperName(functionAnalyzer.GetMemberdef());
}

// =================================================================================

string GenerateWrapper(const GlobalFunctionAnalyzer& functionAnalyzer, vector<shared_ptr<FuncParamConv> >& convertedParams, shared_ptr<FuncReturnTypeConv> convertedReturn)
{
    string result;
    
    result = "static " + convertedReturn->glueReturnType_ + " " + GenerateWrapperName(functionAnalyzer) + "(";

    for (size_t i = 0; i < convertedParams.size(); i++)
    {
        if (i != 0)
            result += ", ";

        result += convertedParams[i]->cppType_ + " " + convertedParams[i]->inputVarName_;
    }

    result +=
        ")\n"
        "{\n";

    for (size_t i = 0; i < convertedParams.size(); i++)
        result += convertedParams[i]->glue_;

    if (convertedReturn->glueReturnType_ != "void")
        result += "    " + functionAnalyzer.GetReturnType().ToString() + " result = ";
    else
        result += "    ";

    result += functionAnalyzer.GetName() + "(";

    for (size_t i = 0; i < convertedParams.size(); i++)
    {
        if (i != 0)
            result += ", ";

        result += convertedParams[i]->convertedVarName_;
    }

    result += ");\n";

    if (convertedReturn->glueReturnType_ != "void")
        result += "    " + convertedReturn->glueReturn_;

    result += "}";

    return result;
}

string GenerateWrapper(const ClassStaticFunctionAnalyzer& functionAnalyzer, vector<shared_ptr<FuncParamConv> >& convertedParams, shared_ptr<FuncReturnTypeConv> convertedReturn)
{
    string result;
    
    string insideDefine = InsideDefine(functionAnalyzer.GetHeaderFile());

    if (!insideDefine.empty())
        result += "#ifdef " + insideDefine + "\n";

    result +=
        "// " + functionAnalyzer.GetLocation() + "\n"
        "static " + convertedReturn->glueReturnType_ + " " + GenerateWrapperName(functionAnalyzer) + "(";

    for (size_t i = 0; i < convertedParams.size(); i++)
    {
        if (i != 0)
            result += ", ";

        result += convertedParams[i]->cppType_ + " " + convertedParams[i]->inputVarName_;
    }

    result +=
        ")\n"
        "{\n";

    for (size_t i = 0; i < convertedParams.size(); i++)
        result += convertedParams[i]->glue_;

    if (convertedReturn->glueReturnType_ != "void")
        result += "    " + functionAnalyzer.GetReturnType().ToString() + " result = ";
    else
        result += "    ";

    result += functionAnalyzer.GetClassName() + "::" + functionAnalyzer.GetName() + "(";

    for (size_t i = 0; i < convertedParams.size(); i++)
    {
        if (i != 0)
            result += ", ";

        result += convertedParams[i]->convertedVarName_;
    }

    result += ");\n";

    if (convertedReturn->glueReturnType_ != "void")
        result += "    " + convertedReturn->glueReturn_;

    result += "}\n";

    if (!insideDefine.empty())
        result += "#endif\n";

    result += "\n";

    return result;
}

string GenerateWrapper(const ClassFunctionAnalyzer& functionAnalyzer, bool templateVersion, vector<shared_ptr<FuncParamConv> >& convertedParams, shared_ptr<FuncReturnTypeConv> convertedReturn)
{
    string result;

    string insideDefine = InsideDefine(functionAnalyzer.GetClass().GetHeaderFile());

    if (!insideDefine.empty())
        result += "#ifdef " + insideDefine + "\n";

    result +=
        "// " + functionAnalyzer.GetLocation() + "\n"
        "static " + convertedReturn->glueReturnType_ + " " + GenerateWrapperName(functionAnalyzer, templateVersion) + "(" + functionAnalyzer.GetClassName() + "* ptr";

    for (size_t i = 0; i < convertedParams.size(); i++)
        result += ", " + convertedParams[i]->cppType_ + " " + convertedParams[i]->inputVarName_;

    result +=
        ")\n"
        "{\n";

    for (size_t i = 0; i < convertedParams.size(); i++)
        result += convertedParams[i]->glue_;

    if (convertedReturn->glueReturnType_ != "void")
        result += "    " + functionAnalyzer.GetReturnType().ToString() + " result = ";
    else
        result += "    ";

    result += "ptr->" + functionAnalyzer.GetName() + "(";

    for (size_t i = 0; i < convertedParams.size(); i++)
    {
        if (i != 0)
            result += ", ";

        result += convertedParams[i]->convertedVarName_;
    }

    result += ");\n";

    if (convertedReturn->glueReturnType_ != "void")
        result += "    " + convertedReturn->glueReturn_;

    result += "}\n";

    if (!insideDefine.empty())
        result += "#endif\n";

    result += "\n";

    return result;
}

// =================================================================================

string Generate_asFUNCTIONPR(const GlobalFunctionAnalyzer& functionAnalyzer)
{
    string functionName = functionAnalyzer.GetName();
    string cppParams = "(" + JoinParamsTypes(functionAnalyzer.GetMemberdef(), functionAnalyzer.GetSpecialization()) + ")";
    string returnType = functionAnalyzer.GetReturnType().ToString();
    return "asFUNCTIONPR(" + functionName + ", " + cppParams + ", " + returnType + ")";
}

string Generate_asFUNCTIONPR(const ClassStaticFunctionAnalyzer& functionAnalyzer)
{
    string className = functionAnalyzer.GetClassName();
    string functionName = functionAnalyzer.GetName();
    string cppParams = "(" + JoinParamsTypes(functionAnalyzer.GetMemberdef(), functionAnalyzer.GetSpecialization()) + ")";
    string returnType = functionAnalyzer.GetReturnType().ToString();
    return "asFUNCTIONPR(" + className + "::" + functionName + ", " + cppParams + ", " + returnType + ")";
}

string Generate_asMETHODPR(const ClassFunctionAnalyzer& functionAnalyzer, bool templateVersion)
{
    string className = functionAnalyzer.GetClassName();
    string functionName = functionAnalyzer.GetName();

    string cppParams = "(" + JoinParamsTypes(functionAnalyzer.GetMemberdef(), functionAnalyzer.GetSpecialization()) + ")";

    if (functionAnalyzer.IsConst())
        cppParams += " const";

    string returnType = functionAnalyzer.GetReturnType().ToString();
    
    if (templateVersion)
        return "asMETHODPR(T, " + functionName + ", " + cppParams + ", " + returnType + ")";
    else
        return "asMETHODPR(" + className + ", " + functionName + ", " + cppParams + ", " + returnType + ")";
}

} // namespace ASBindingGenerator
