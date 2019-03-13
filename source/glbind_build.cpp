#include "external/tinyxml2.cpp"
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <assert.h>

#define GLB_BUILD_XML_PATH_GL   "../../resources/gl.xml"
#define GLB_BUILD_XML_PATH_WGL  "../../resources/wgl.xml"
#define GLB_BUILD_XML_PATH_GLX  "../../resources/glx.xml"
#define GLB_BUILD_TEMPLATE_PATH "../../source/glbind_template.h"

// Edit these if you want to blacklist specific extensions.
static const char* g_BlacklistedExtensions[] = {
    "GLX_SGIX_dmbuffer",        // Don't know what the DMparams and DMbuffer types are. I assume "DM" means "direct memory"?
    "GLX_SGIX_video_source"     // Don't know what the VLServer, VLPath and VLNode types are.
};

// Edit these if you want to blacklist specific extension vendors.
static const char* g_BlacklistedExtensionVendors[] = {
    /*"SGIX"*/
    ""
};


typedef int glbResult;
#define GLB_SUCCESS                 0
#define GLB_ALREADY_PROCESSED       1   /* Not an error. Used to indicate that a type has already been output. */
#define GLB_ERROR                   -1
#define GLB_INVALID_ARGS            -2
#define GLB_OUT_OF_MEMORY           -3
#define GLB_FILE_TOO_BIG            -4
#define GLB_FAILED_TO_OPEN_FILE     -5
#define GLB_FAILED_TO_READ_FILE     -6
#define GLB_FAILED_TO_WRITE_FILE    -7

std::string glbLTrim(const std::string &s)
{
    std::string result = s;
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](int character) { return !std::isspace(character); }));
    return result;
}

std::string glbRTrim(const std::string &s)
{
    std::string result = s;
    result.erase(std::find_if(result.rbegin(), result.rend(), [](int character) { return !std::isspace(character); }).base(), result.end());
    return result;
}

std::string glbTrim(const std::string &s)
{
    return glbLTrim(glbRTrim(s));
}

std::string glbReplaceAll(const std::string &source, const std::string &from, const std::string &to)
{
    std::string result;
    std::string::size_type lastPos = 0;

    for (;;) {
        std::string::size_type findPos = source.find(from, lastPos);
        if (findPos == std::string::npos) {
            break;
        }

        result.append(source, lastPos, findPos - lastPos);
        result.append(to);
        lastPos = findPos + from.length();
    }

    result.append(source.substr(lastPos));
    return result;
}

void glbReplaceAllInline(std::string &source, const std::string &from, const std::string &to)
{
    source = glbReplaceAll(source, from, to);
}

std::string glbToUpper(const std::string &source)
{
    // Quick and dirty...
    std::string upper;
    for (size_t i = 0; i < source.size(); ++i) {
        upper += (char)std::toupper(source[i]);
    }
    return upper;
}

bool glbContains(const std::string &source, const char* other)
{
    return source.find(other) != std::string::npos;
}



glbResult glbFOpen(const char* filePath, const char* openMode, FILE** ppFile)
{
    if (filePath == NULL || openMode == NULL || ppFile == NULL) {
        return GLB_INVALID_ARGS;
    }

#if defined(_MSC_VER) && _MSC_VER > 1400   /* 1400 = Visual Studio 2005 */
    {
        if (fopen_s(ppFile, filePath, openMode) != 0) {
            return GLB_FAILED_TO_OPEN_FILE;
        }
    }
#else
    {
        FILE* pFile = fopen(filePath, openMode);
        if (pFile == NULL) {
            return GLB_FAILED_TO_OPEN_FILE;
        }

        *ppFile = pFile;
    }
#endif

    return GLB_SUCCESS;
}

glbResult glbOpenAndReadFileWithExtraData(const char* filePath, size_t* pFileSizeOut, void** ppFileData, size_t extraBytes)
{
    glbResult result;
    FILE* pFile;
    uint64_t fileSize;
    void* pFileData;
    size_t bytesRead;

    /* Safety. */
    if (pFileSizeOut) *pFileSizeOut = 0;
    if (ppFileData) *ppFileData = NULL;

    if (filePath == NULL) {
        return GLB_INVALID_ARGS;
    }

    result = glbFOpen(filePath, "rb", &pFile);
    if (result != GLB_SUCCESS) {
        return GLB_FAILED_TO_OPEN_FILE;
    }

    fseek(pFile, 0, SEEK_END);
    fileSize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    if (fileSize + extraBytes > SIZE_MAX) {
        fclose(pFile);
        return GLB_FILE_TOO_BIG;
    }

    pFileData = malloc((size_t)fileSize + extraBytes);    /* <-- Safe cast due to the check above. */
    if (pFileData == NULL) {
        fclose(pFile);
        return GLB_OUT_OF_MEMORY;
    }

    bytesRead = fread(pFileData, 1, (size_t)fileSize, pFile);
    if (bytesRead != fileSize) {
        free(pFileData);
        fclose(pFile);
        return GLB_FAILED_TO_READ_FILE;
    }

    fclose(pFile);

    if (pFileSizeOut) {
        *pFileSizeOut = (size_t)fileSize;
    }

    if (ppFileData) {
        *ppFileData = pFileData;
    } else {
        free(pFileData);
    }

    return GLB_SUCCESS;
}

glbResult glbOpenAndReadFile(const char* filePath, size_t* pFileSizeOut, void** ppFileData)
{
    return glbOpenAndReadFileWithExtraData(filePath, pFileSizeOut, ppFileData, 0);
}

glbResult glbOpenAndReadTextFile(const char* filePath, size_t* pFileSizeOut, char** ppFileData)
{
    size_t fileSize;
    char* pFileData;
    glbResult result = glbOpenAndReadFileWithExtraData(filePath, &fileSize, (void**)&pFileData, 1);
    if (result != GLB_SUCCESS) {
        return result;
    }

    pFileData[fileSize] = '\0';

    if (pFileSizeOut) {
        *pFileSizeOut = fileSize;
    }

    if (ppFileData) {
        *ppFileData = pFileData;
    } else {
        free(pFileData);
    }

    return GLB_SUCCESS;
}

glbResult glbOpenAndWriteFile(const char* filePath, const void* pData, size_t dataSize)
{
    glbResult result;
    FILE* pFile;

    if (filePath == NULL) {
        return GLB_INVALID_ARGS;
    }

    result = glbFOpen(filePath, "wb", &pFile);
    if (result != GLB_SUCCESS) {
        return GLB_FAILED_TO_OPEN_FILE;
    }

    if (pData != NULL && dataSize > 0) {
        if (fwrite(pData, 1, dataSize, pFile) != dataSize) {
            fclose(pFile);
            return GLB_FAILED_TO_WRITE_FILE;
        }
    }

    fclose(pFile);
    return GLB_SUCCESS;
}

glbResult glbOpenAndWriteTextFile(const char* filePath, const char* text)
{
    if (text == NULL) {
        text = "";
    }

    return glbOpenAndWriteFile(filePath, text, strlen(text));
}



struct glbType
{
    std::string name;       // Can be an attribute of an inner tag.
    std::string valueC;     // The value as C code.
    std::string requires;
};

struct glbEnum
{
    std::string name;
    std::string value;      // Can be an empty string.
    std::string type;
};

struct glbGroup
{
    std::string name;
    std::vector<glbEnum> enums;
};

struct glbEnums
{
    std::string name;
    std::string namespaceAttrib;
    std::string group;
    std::string vendor;
    std::string type;
    std::string start;
    std::string end;
    std::vector<glbEnum> enums;
};

struct glbCommandParam
{
    std::string type;
    std::string typeC;
    std::string name;
    std::string group;  // Attribute.
};

struct glbCommand
{
    std::string returnType;
    std::string returnTypeC;
    std::string name;
    std::vector<glbCommandParam> params;
    std::string alias;
};

struct glbCommands
{
    std::string namespaceAttrib;
    std::vector<glbCommand> commands;
};

struct glbRequire
{
    std::vector<std::string> types;
    std::vector<std::string> enums;
    std::vector<std::string> commands;
};

struct glbFeature
{
    std::string api;
    std::string name;
    std::string number;
    std::vector<glbRequire> requires;
};

struct glbExtension
{
    std::string name;
    std::string supported;
    std::vector<glbRequire> requires;
};

struct glbBuild
{
    std::vector<glbType>      types;
    std::vector<glbGroup>     groups;
    std::vector<glbEnums>     enums;
    std::vector<glbCommands>  commands;
    std::vector<glbFeature>   features;
    std::vector<glbExtension> extensions;

    std::vector<std::string> outputTypes;
    std::vector<std::string> outputEnums;
    std::vector<std::string> outputCommands;
};

glbResult glbBuildParseTypes(glbBuild &context, tinyxml2::XMLNode* pXMLElement)
{
    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        // Ignore <comment> tags.
        if (strcmp(pChildElement->Name(), "comment") == 0) {
            continue;
        }

        const char* name = pChildElement->Attribute("name");
        const char* requires = pChildElement->Attribute("requires");

        glbType type;
        type.name     = (name     != NULL) ? name     : "";
        type.requires = (requires != NULL) ? requires : "";

        // The inner content of the child will contain the C code. We need to parse this by simply appending the text content together.
        for (tinyxml2::XMLNode* pInnerChild = pChild->FirstChild(); pInnerChild != NULL; pInnerChild = pInnerChild->NextSibling()) {
            tinyxml2::XMLElement* pInnerChildElement = pInnerChild->ToElement();
            if (pInnerChildElement != NULL) {
                if (strcmp(pInnerChildElement->Name(), "name") == 0) {
                    type.name = pInnerChildElement->FirstChild()->Value();
                    type.valueC += type.name;
                }
                if (strcmp(pInnerChildElement->Name(), "apientry") == 0) {
                    type.valueC += "APIENTRY";
                }
            } else {
                type.valueC += pInnerChild->Value();
            }
        }

        context.types.push_back(type);
    }

    return GLB_SUCCESS;
}

glbResult glbBuildParseEnum(glbBuild &context, tinyxml2::XMLElement* pXMLElement, glbEnum &theEnum)
{
    (void)context;

    const char* name  = pXMLElement->Attribute("name");
    const char* value = pXMLElement->Attribute("value");
    const char* type  = pXMLElement->Attribute("type");

    theEnum.name  = (name  != NULL) ? name  : "";
    theEnum.value = (value != NULL) ? value : "";
    theEnum.type  = (type  != NULL) ? type  : "";

    return GLB_SUCCESS;
}

glbResult glbBuildParseEnums(glbBuild &context, tinyxml2::XMLElement* pXMLElement)
{
    const char* name   = pXMLElement->Attribute("name");
    const char* namespaceAttrib = pXMLElement->Attribute("namespace");
    const char* group  = pXMLElement->Attribute("group");
    const char* vendor = pXMLElement->Attribute("vendor");
    const char* type   = pXMLElement->Attribute("type");
    const char* start  = pXMLElement->Attribute("start");
    const char* end    = pXMLElement->Attribute("end");

    glbEnums enums;
    enums.name            = (name            != NULL) ? name            : "";
    enums.namespaceAttrib = (namespaceAttrib != NULL) ? namespaceAttrib : "";
    enums.group           = (group           != NULL) ? group           : "";
    enums.vendor          = (vendor          != NULL) ? vendor          : "";
    enums.type            = (type            != NULL) ? type            : "";
    enums.start           = (start           != NULL) ? start           : "";
    enums.end             = (end             != NULL) ? end             : "";

    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        // Ignore <comment> tags.
        if (strcmp(pChildElement->Name(), "enum") == 0) {
            glbEnum theEnum;
            glbResult result = glbBuildParseEnum(context, pChildElement, theEnum);
            if (result == GLB_SUCCESS) {
                enums.enums.push_back(theEnum);
            }
        }
    }

    context.enums.push_back(enums);

    return GLB_SUCCESS;
}

glbResult glbBuildParseGroup(glbBuild &context, tinyxml2::XMLElement* pXMLElement, glbGroup &group)
{
    const char* name = pXMLElement->Attribute("name");

    group.name = (name != NULL) ? name : "";

    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        if (strcmp(pChildElement->Name(), "enum") == 0) {
            glbEnum theEnum;
            glbResult result = glbBuildParseEnum(context, pChildElement, theEnum);
            if (result == GLB_SUCCESS) {
                group.enums.push_back(theEnum);
            }
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildParseGroups(glbBuild &context, tinyxml2::XMLElement* pXMLElement)
{
    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        if (strcmp(pChildElement->Name(), "group") == 0) {
            glbGroup group;
            glbResult result = glbBuildParseGroup(context, pChildElement, group);
            if (result == GLB_SUCCESS) {
                context.groups.push_back(group);
            }
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildParseTypeNamePair(tinyxml2::XMLElement* pXMLElement, std::string &type, std::string &typeC, std::string &name)
{
    // Everything up to the name is the type. We set "type" to the value inside the <type> or <ptype> tag, if any. "typeC" will be set to the
    // whole type up to, but not including, the <name> tag.
    type  = "";
    typeC = "";
    name  = "";

    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement != NULL) {
            if (strcmp(pChildElement->Name(), "name") == 0) {
                name = pChildElement->FirstChild()->Value();
                break;
            } else {
                typeC += pChildElement->FirstChild()->Value();
                if (strcmp(pChildElement->Name(), "type") == 0 || strcmp(pChildElement->Name(), "ptype") == 0) {
                    type = pChildElement->FirstChild()->Value();
                }
            }
        } else {
            typeC += pChild->Value();
        }
    }

    typeC = glbTrim(typeC);

    return GLB_SUCCESS;
}

glbResult glbBuildParseCommandParam(glbBuild &context, tinyxml2::XMLElement* pXMLElement, glbCommandParam &param)
{
    (void)context;

    return glbBuildParseTypeNamePair(pXMLElement, param.type, param.typeC, param.name);
}

glbResult glbBuildParseCommand(glbBuild &context, tinyxml2::XMLElement* pXMLElement, glbCommand &command)
{
    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        if (strcmp(pChildElement->Name(), "proto") == 0) {
            glbBuildParseTypeNamePair(pChildElement, command.returnType, command.returnTypeC, command.name);
        }

        if (strcmp(pChildElement->Name(), "param") == 0) {
            glbCommandParam param;
            glbResult result = glbBuildParseCommandParam(context, pChildElement, param);
            if (result != GLB_SUCCESS) {
                return result;
            }

            command.params.push_back(param);
        }

        if (strcmp(pChildElement->Name(), "alias") == 0) {
            const char* alias = pChildElement->Attribute("name");
            command.alias = (alias != NULL) ? alias : "";
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildParseCommands(glbBuild &context, tinyxml2::XMLElement* pXMLElement)
{
    glbCommands commands;

    const char* namespaceAttrib = pXMLElement->Attribute("namespace");
    commands.namespaceAttrib = (namespaceAttrib != NULL) ? namespaceAttrib : "";

    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        if (strcmp(pChildElement->Name(), "command") == 0) {
            glbCommand command;
            glbResult result = glbBuildParseCommand(context, pChildElement, command);
            if (result == GLB_SUCCESS) {
                commands.commands.push_back(command);
            }
        }
    }

    context.commands.push_back(commands);

    return GLB_SUCCESS;
}

glbResult glbBuildParseRequire(glbBuild &context, tinyxml2::XMLElement* pXMLElement, glbRequire &require)
{
    (void)context;

    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        if (strcmp(pChildElement->Name(), "type") == 0) {
            require.types.push_back(pChildElement->Attribute("name"));
        }

        if (strcmp(pChildElement->Name(), "enum") == 0) {
            require.enums.push_back(pChildElement->Attribute("name"));
        }

        if (strcmp(pChildElement->Name(), "command") == 0) {
            require.commands.push_back(pChildElement->Attribute("name"));
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildParseFeature(glbBuild &context, tinyxml2::XMLElement* pXMLElement)
{
    glbFeature feature;

    const char* api    = pXMLElement->Attribute("api");
    const char* name   = pXMLElement->Attribute("name");
    const char* number = pXMLElement->Attribute("number");

    feature.api    = (api    != NULL) ? api    : "";
    feature.name   = (name   != NULL) ? name   : "";
    feature.number = (number != NULL) ? number : "";

    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        if (strcmp(pChildElement->Name(), "require") == 0) {
            glbRequire require;
            glbResult result = glbBuildParseRequire(context, pChildElement, require);
            if (result != GLB_SUCCESS) {
                return result;
            }

            feature.requires.push_back(require);
        }
    }

    context.features.push_back(feature);

    return GLB_SUCCESS;
}


glbResult glbBuildParseExtension(glbBuild &context, tinyxml2::XMLElement* pXMLElement, glbExtension &extension)
{
    const char* name      = pXMLElement->Attribute("name");
    const char* supported = pXMLElement->Attribute("supported");

    extension.name      = (name      != NULL) ? name      : "";
    extension.supported = (supported != NULL) ? supported : "";

    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        if (strcmp(pChildElement->Name(), "require") == 0) {
            glbRequire require;
            glbResult result = glbBuildParseRequire(context, pChildElement, require);
            if (result != GLB_SUCCESS) {
                return result;
            }

            extension.requires.push_back(require);
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildParseExtensions(glbBuild &context, tinyxml2::XMLElement* pXMLElement)
{
    for (tinyxml2::XMLNode* pChild = pXMLElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;
        }

        if (strcmp(pChildElement->Name(), "extension") == 0) {
            glbExtension extension;
            glbResult result = glbBuildParseExtension(context, pChildElement, extension);
            if (result != GLB_SUCCESS) {
                return result;
            }

            context.extensions.push_back(extension);
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildLoadXML(glbBuild &context, tinyxml2::XMLDocument &doc)
{
    // The root node is the <registry> node.
    tinyxml2::XMLElement* pRoot = doc.RootElement();
    if (pRoot == NULL) {
        printf("Failed to retrieve root node.\n");
        return -1;
    }

    if (strcmp(pRoot->Name(), "registry") != 0) {
        printf("Unexpected root node. Expecting \"registry\", but got \"%s\"", pRoot->Name());
        return -1;
    }

    for (tinyxml2::XMLNode* pChild = pRoot->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
        tinyxml2::XMLElement* pChildElement = pChild->ToElement();
        if (pChildElement == NULL) {
            continue;   // Could be a comment. In any case we don't care about anything that's not in a child node.
        }

        if (strcmp(pChildElement->Name(), "types") == 0) {
            glbBuildParseTypes(context, pChildElement);
        }
        if (strcmp(pChildElement->Name(), "groups") == 0) {
            glbBuildParseGroups(context, pChildElement);
        }
        if (strcmp(pChildElement->Name(), "enums") == 0) {
            glbBuildParseEnums(context, pChildElement);
        }
        if (strcmp(pChildElement->Name(), "commands") == 0) {
            glbBuildParseCommands(context, pChildElement);
        }
        if (strcmp(pChildElement->Name(), "feature") == 0) {
            glbBuildParseFeature(context, pChildElement);
        }
        if (strcmp(pChildElement->Name(), "extensions") == 0) {
            glbBuildParseExtensions(context, pChildElement);
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildLoadXMLFile(glbBuild &context, const char* filePath)
{
    tinyxml2::XMLDocument docGL;
    tinyxml2::XMLError xmlError = docGL.LoadFile(filePath);
    if (xmlError != tinyxml2::XML_SUCCESS) {
        printf("Failed to parse %s\n", filePath);
        return (int)xmlError;
    }

    return glbBuildLoadXML(context, docGL);
}


bool glbDoesExtensionBelongToVendor(const std::string &extensionName, const std::string &vendor)
{
    return extensionName.find("_" + vendor + "_") != std::string::npos;
}

bool glbIsExtensionBlackListed(const char* extensionName)
{
    for (auto blacklistedExtension : g_BlacklistedExtensions) {
        if (strcmp(blacklistedExtension, extensionName) == 0) {
            return true;
        }
    }

    for (auto vendor : g_BlacklistedExtensionVendors) {
        if (glbDoesExtensionBelongToVendor(extensionName, vendor)) {
            return true;
        }
    }

    return false;
}

bool glbBuildIsCommandIgnored(const char* commandName)
{
    // The following commands should not be included in anything as they are not strictly OpenGL related.
    static const char* ignoredCommands[] = {
        "ChoosePixelFormat",
        "DescribePixelFormat",
        "GetEnhMetaFilePixelFormat",
        "GetPixelFormat",
        "SetPixelFormat",
        "SwapBuffers",
        "wglUseFontBitmaps",
        "wglUseFontOutlines",
    };

    for (size_t i = 0; i < sizeof(ignoredCommands)/sizeof(ignoredCommands[0]); ++i) {
        if (strcmp(ignoredCommands[i], commandName) == 0) {
            return true;
        }
    }

    return false;
}


bool glbBuildHasTypeBeenOutput(glbBuild &context, const char* typeName)
{
    for (size_t iType = 0; iType < context.outputTypes.size(); ++iType) {
        if (context.outputTypes[iType] == typeName) {
            return true;
        }
    }

    return false;   /* Not found. */
}

bool glbBuildHasCommandBeenOutput(glbBuild &context, const char* commandName)
{
    for (size_t iCommand = 0; iCommand < context.outputCommands.size(); ++iCommand) {
        if (context.outputCommands[iCommand] == commandName) {
            return true;
        }
    }

    return false;   /* Not found. */
}

glbResult glbBuildFindType(glbBuild &context, const char* typeName, glbType** ppType)
{
    *ppType = NULL;

    for (size_t iType = 0; iType < context.types.size(); ++iType) {
        if (context.types[iType].name == typeName) {
            *ppType = &context.types[iType];
            return GLB_SUCCESS;
        }
    }
    
    return GLB_ERROR;
}

glbResult glbBuildFindEnum(glbBuild &context, const char* enumName, glbEnum** ppEnum)
{
    *ppEnum = NULL;

    for (size_t iEnums = 0; iEnums < context.enums.size(); ++iEnums) {
        glbEnums &enums = context.enums[iEnums];
        for (size_t iEnum = 0; iEnum < enums.enums.size(); ++iEnum) {
            if (context.enums[iEnums].enums[iEnum].name == enumName) {
                *ppEnum = &context.enums[iEnums].enums[iEnum];
                return GLB_SUCCESS;
            }
        }
    }
    
    return GLB_ERROR;
}

glbResult glbBuildFindCommand(glbBuild &context, const char* commandName, glbCommand** ppCommand)
{
    *ppCommand = NULL;

    for (size_t iCommands = 0; iCommands < context.commands.size(); ++iCommands) {
        glbCommands &commands = context.commands[iCommands];
        for (size_t iCommand = 0; iCommand < commands.commands.size(); ++iCommand) {
            if (context.commands[iCommands].commands[iCommand].name == commandName) {
                *ppCommand = &context.commands[iCommands].commands[iCommand];
                return GLB_SUCCESS;
            }
        }
    }
    
    return GLB_ERROR;
}


glbResult glbBuildGenerateCode_C_Main_Type(glbBuild &context, const char* typeName, std::string &codeOut)
{
    // Special case for khrplatform. We don't want to include this because we don't use khrplatform.h. Just pretend it's already been output.
    if (strcmp(typeName, "khrplatform") == 0) {
        return GLB_ALREADY_PROCESSED;
    }

    // We only output the type if it hasn't already been output.
    if (!glbBuildHasTypeBeenOutput(context, typeName)) {
        glbType* pType;
        glbResult result = glbBuildFindType(context, typeName, &pType);
        if (result != GLB_SUCCESS) {
            return result;
        }

        if (pType->valueC != "") {
            codeOut += pType->valueC + "\n";
        }

        context.outputTypes.push_back(typeName);
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_Main_RequireTypes(glbBuild &context, glbRequire &require, std::string &codeOut)
{
    glbResult result;

    // Standalone types.
    for (size_t iType = 0; iType < require.types.size(); ++iType) {
        result = glbBuildGenerateCode_C_Main_Type(context, require.types[iType].c_str(), codeOut);
        if (result != GLB_SUCCESS && result != GLB_ALREADY_PROCESSED) {
            return result;
        }
    }

    // Required types.
    for (size_t iCommand = 0; iCommand < require.commands.size(); ++iCommand) {
        glbCommand* pCommand;
        result = glbBuildFindCommand(context, require.commands[iCommand].c_str(), &pCommand);
        if (result != GLB_SUCCESS) {
            return result;
        }

        if (pCommand->returnType != "") {
            glbBuildGenerateCode_C_Main_Type(context, pCommand->returnType.c_str(), codeOut);
        }

        for (size_t iParam = 0; iParam < pCommand->params.size(); ++iParam) {
            glbCommandParam &param = pCommand->params[iParam];
            if (param.type != "") {
                glbBuildGenerateCode_C_Main_Type(context, param.type.c_str(), codeOut);
            }
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_Main_RequireEnums(glbBuild &context, glbRequire &require, std::string &codeOut)
{
    glbResult result;

    for (size_t iEnum = 0; iEnum < require.enums.size(); ++iEnum) {
        glbEnum* pEnum;
        result = glbBuildFindEnum(context, require.enums[iEnum].c_str(), &pEnum);
        if (result != GLB_SUCCESS) {
            return result;
        }

        codeOut += "#define " + pEnum->name + " " + pEnum->value + "\n";
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_Main_RequireCommands(glbBuild &context, glbRequire &require, std::string &codeOut)
{
    glbResult result;

    for (size_t iCommand = 0; iCommand < require.commands.size(); ++iCommand) {
        const char* commandName = require.commands[iCommand].c_str();
        if (!glbBuildHasCommandBeenOutput(context, commandName)) {
            glbCommand* pCommand;
            result = glbBuildFindCommand(context, require.commands[iCommand].c_str(), &pCommand);
            if (result != GLB_SUCCESS) {
                return result;
            }

            codeOut += "typedef " + pCommand->returnTypeC + " (APIENTRYP PFN" + glbToUpper(pCommand->name) + "PROC)(";
            if (pCommand->params.size() > 0) {
                for (size_t iParam = 0; iParam < pCommand->params.size(); ++iParam) {
                    if (iParam != 0) {
                        codeOut += ", ";
                    }
                    codeOut += pCommand->params[iParam].typeC + " " + pCommand->params[iParam].name;
                }
            } else {
                codeOut += "void";  // we need to use "func(void)" syntax for compatibility with older versions of C.
            }
            codeOut += ");\n";

            context.outputCommands.push_back(commandName);
        }
    }

    return GLB_SUCCESS;
}


glbResult glbBuildGenerateCode_C_Main_Feature(glbBuild &context, glbFeature &feature, std::string &codeOut)
{
    codeOut += "#ifndef " + feature.name + "\n";
    codeOut += "#define " + feature.name + " 1\n";
    {
        // Types.
        for (size_t iRequire = 0; iRequire < feature.requires.size(); ++iRequire) {
            glbResult result = glbBuildGenerateCode_C_Main_RequireTypes(context, feature.requires[iRequire], codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }       
        }

        // Enums.
        for (size_t iRequire = 0; iRequire < feature.requires.size(); ++iRequire) {
            glbResult result = glbBuildGenerateCode_C_Main_RequireEnums(context, feature.requires[iRequire], codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }       
        }

        // Commands.
        for (size_t iRequire = 0; iRequire < feature.requires.size(); ++iRequire) {
            glbResult result = glbBuildGenerateCode_C_Main_RequireCommands(context, feature.requires[iRequire], codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }       
        }
    }
    codeOut += "#endif /* " + feature.name + " */\n";

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_Main_FeaturesByAPI(glbBuild &context, const char* api, std::string &codeOut)
{
    int counter = 0;    // Only used for knowing whether or not a new line should be added.

    for (size_t iFeature = 0; iFeature < context.features.size(); ++iFeature) {
        glbFeature &feature = context.features[iFeature];
        if (feature.api == api) {
            if (counter > 0) {
                codeOut += "\n";
            }

            glbResult result = glbBuildGenerateCode_C_Main_Feature(context, feature, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }

            counter += 1;
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_Main_Extension(glbBuild &context, glbExtension &extension, std::string &codeOut)
{
    codeOut += "#ifndef " + extension.name + "\n";
    codeOut += "#define " + extension.name + " 1\n";
    {
        // Types.
        for (size_t iRequire = 0; iRequire < extension.requires.size(); ++iRequire) {
            glbResult result = glbBuildGenerateCode_C_Main_RequireTypes(context, extension.requires[iRequire], codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }       
        }

        // Enums.
        for (size_t iRequire = 0; iRequire < extension.requires.size(); ++iRequire) {
            glbResult result = glbBuildGenerateCode_C_Main_RequireEnums(context, extension.requires[iRequire], codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }       
        }

        // Commands.
        for (size_t iRequire = 0; iRequire < extension.requires.size(); ++iRequire) {
            glbResult result = glbBuildGenerateCode_C_Main_RequireCommands(context, extension.requires[iRequire], codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }       
        }
    }
    codeOut += "#endif /* " + extension.name + " */\n";

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_Main(glbBuild &context, std::string &codeOut)
{
    glbResult result;
    int counter;    // Used for tracking whether or not new lines need to be added.

    // Feature order is the following.
    //  - gl
    //  - wgl
    //  - glx
    result = glbBuildGenerateCode_C_Main_FeaturesByAPI(context, "gl", codeOut);
    if (result != GLB_SUCCESS) {
        return result;
    }

    codeOut += "\n#if defined(GLBIND_WGL)\n";
    result = glbBuildGenerateCode_C_Main_FeaturesByAPI(context, "wgl", codeOut);
    if (result != GLB_SUCCESS) {
        return result;
    }
    codeOut += "#endif /* GLBIND_WGL */\n";

    codeOut += "\n#if defined(GLBIND_GLX)\n";
    result = glbBuildGenerateCode_C_Main_FeaturesByAPI(context, "glx", codeOut);
    if (result != GLB_SUCCESS) {
        return result;
    }
    codeOut += "#endif /* GLBIND_GLX */\n";

    // TODO: All other APIs no in gl, wgl and glx (gles2, etc.)?


    // Now we need to do extensions. For cleanliness we want to group extensions. We do gl/glcore first, then wgl, then glx.
    counter = 0;
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && (extension.supported == "gl" || glbContains(extension.supported, "gl|") || glbContains(extension.supported, "glcore"))) {
            if (counter > 0) {
                codeOut += "\n";
            }
            counter += 1;

            result = glbBuildGenerateCode_C_Main_Extension(context, extension, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }

    // WGL extensions.
    codeOut += "\n#if defined(GLBIND_WGL)\n";
    counter = 0;
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && glbContains(extension.supported, "wgl")) {
            if (counter > 0) {
                codeOut += "\n";
            }
            counter += 1;

            result = glbBuildGenerateCode_C_Main_Extension(context, extension, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_WGL */\n";

    // GLX extensions.
    codeOut += "\n#if defined(GLBIND_GLX)\n";
    counter = 0;
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && glbContains(extension.supported, "glx")) {
            if (counter > 0) {
                codeOut += "\n";
            }
            counter += 1;

            result = glbBuildGenerateCode_C_Main_Extension(context, extension, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_GLX */\n";

    return GLB_SUCCESS;
}



glbResult glbBuildGenerateCode_C_FuncPointersDeclGlobal_RequireCommands(glbBuild &context, int indentation, const glbRequire &require, std::vector<std::string> &processedCommands, std::string &codeOut)
{
    glbResult result;

    for (size_t iCommand = 0; iCommand < require.commands.size(); ++iCommand) {
        std::string commandName = require.commands[iCommand];
        if (!glbBuildIsCommandIgnored(commandName.c_str()) && std::find(processedCommands.begin(), processedCommands.end(), commandName) == processedCommands.end()) {
            glbCommand* pCommand;
            result = glbBuildFindCommand(context, require.commands[iCommand].c_str(), &pCommand);
            if (result != GLB_SUCCESS) {
                return result;
            }

            for (int i = 0; i < indentation; ++i) {
                codeOut += " ";
            }
            codeOut += "PFN" + glbToUpper(pCommand->name) + "PROC " + pCommand->name + ";\n";

            processedCommands.push_back(commandName);
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_FuncPointersDeclGlobal_Extension(glbBuild &context, int indentation, const glbExtension &extension, std::vector<std::string> &processedCommands, std::string &codeOut)
{
    for (size_t iRequire = 0; iRequire < extension.requires.size(); ++iRequire) {
        glbResult result = glbBuildGenerateCode_C_FuncPointersDeclGlobal_RequireCommands(context, indentation, extension.requires[iRequire], processedCommands, codeOut);
        if (result != GLB_SUCCESS) {
            return result;
        }       
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_FuncPointersDeclGlobal_Feature(glbBuild &context, int indentation, const glbFeature &feature, std::vector<std::string> &processedCommands, std::string &codeOut)
{
    for (size_t iRequire = 0; iRequire < feature.requires.size(); ++iRequire) {
        glbResult result = glbBuildGenerateCode_C_FuncPointersDeclGlobal_RequireCommands(context, indentation, feature.requires[iRequire], processedCommands, codeOut);
        if (result != GLB_SUCCESS) {
            return result;
        }       
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_FuncPointersDeclGlobal(glbBuild &context, int indentation, std::string &codeOut)
{
    glbResult result;
    std::vector<std::string> processedCommands;
    bool isGlobalScope = (indentation == 0); // If the indentation is 0 it means we're generating the global symbols. Bit of a hack, but it works.

    // GL features.
    for (size_t iFeature = 0; iFeature < context.features.size(); ++iFeature) {
        glbFeature &feature = context.features[iFeature];
        if (feature.api == "gl") {
            result = glbBuildGenerateCode_C_FuncPointersDeclGlobal_Feature(context, indentation, context.features[iFeature], processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }

    // NOTE: Some headers define their own symbols for some APIs, such as the common WGL functions (wglCreateContext, etc.). This causes compiler errors
    // so I'm working around this by simply not outputting them to global scope. We do, however, need these to be included in the GLBapi structure.

    // WGL features.
    if (!isGlobalScope) {
        codeOut += "#if defined(GLBIND_WGL)\n";
        for (size_t iFeature = 0; iFeature < context.features.size(); ++iFeature) {
            glbFeature &feature = context.features[iFeature];
            if (feature.api == "wgl") {
                result = glbBuildGenerateCode_C_FuncPointersDeclGlobal_Feature(context, indentation, context.features[iFeature], processedCommands, codeOut);
                if (result != GLB_SUCCESS) {
                    return result;
                }
            }
        }
        codeOut += "#endif /* GLBIND_WGL */\n";
    }

    if (!isGlobalScope) {
        codeOut += "#if defined(GLBIND_GLX)\n";
        for (size_t iFeature = 0; iFeature < context.features.size(); ++iFeature) {
            glbFeature &feature = context.features[iFeature];
            if (feature.api == "glx") {
                result = glbBuildGenerateCode_C_FuncPointersDeclGlobal_Feature(context, indentation, context.features[iFeature], processedCommands, codeOut);
                if (result != GLB_SUCCESS) {
                    return result;
                }
            }
        }
        codeOut += "#endif /* GLBIND_GLX */\n";
    }

    // GL extensions.
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && (extension.supported == "gl" || glbContains(extension.supported, "gl|") || glbContains(extension.supported, "glcore"))) {
            result = glbBuildGenerateCode_C_FuncPointersDeclGlobal_Extension(context, indentation, extension, processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }

    // WGL extensions.
    codeOut += "#if defined(GLBIND_WGL)\n";
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && glbContains(extension.supported, "wgl")) {
            result = glbBuildGenerateCode_C_FuncPointersDeclGlobal_Extension(context, indentation, extension, processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_WGL */\n";

    // GLX extensions.
    codeOut += "#if defined(GLBIND_GLX)\n";
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && glbContains(extension.supported, "glx")) {
            result = glbBuildGenerateCode_C_FuncPointersDeclGlobal_Extension(context, indentation, extension, processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_GLX */";

    return GLB_SUCCESS;
}


glbResult glbBuildGenerateCode_C_InitCurrentContextAPI_RequireCommands(glbBuild &context, const glbRequire &require, std::vector<std::string> &processedCommands, std::string &codeOut)
{
    glbResult result;

    for (size_t iCommand = 0; iCommand < require.commands.size(); ++iCommand) {
        std::string commandName = require.commands[iCommand];
        if (!glbBuildIsCommandIgnored(commandName.c_str()) && std::find(processedCommands.begin(), processedCommands.end(), commandName) == processedCommands.end()) {
            glbCommand* pCommand;
            result = glbBuildFindCommand(context, require.commands[iCommand].c_str(), &pCommand);
            if (result != GLB_SUCCESS) {
                return result;
            }

            codeOut += "    pAPI->" + pCommand->name + " = (PFN" + glbToUpper(pCommand->name) + "PROC)glbGetProcAddress(\"" + pCommand->name + "\");\n";

            processedCommands.push_back(commandName);
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_InitCurrentContextAPI_Feature(glbBuild &context, const glbFeature &feature, std::vector<std::string> &processedCommands, std::string &codeOut)
{
    for (size_t iRequire = 0; iRequire < feature.requires.size(); ++iRequire) {
        glbResult result = glbBuildGenerateCode_C_InitCurrentContextAPI_RequireCommands(context, feature.requires[iRequire], processedCommands, codeOut);
        if (result != GLB_SUCCESS) {
            return result;
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_InitCurrentContextAPI_Extension(glbBuild &context, const glbExtension &extension, std::vector<std::string> &processedCommands, std::string &codeOut)
{
    for (size_t iRequire = 0; iRequire < extension.requires.size(); ++iRequire) {
        glbResult result = glbBuildGenerateCode_C_InitCurrentContextAPI_RequireCommands(context, extension.requires[iRequire], processedCommands, codeOut);
        if (result != GLB_SUCCESS) {
            return result;
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_InitCurrentContextAPI(glbBuild &context, std::string &codeOut)
{
    glbResult result;
    std::vector<std::string> processedCommands;

    // GL features.
    for (size_t iFeature = 0; iFeature < context.features.size(); ++iFeature) {
        glbFeature &feature = context.features[iFeature];
        if (feature.api == "gl") {
            result = glbBuildGenerateCode_C_InitCurrentContextAPI_Feature(context, context.features[iFeature], processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }

    // WGL features.
    codeOut += "#if defined(GLBIND_WGL)\n";
    for (size_t iFeature = 0; iFeature < context.features.size(); ++iFeature) {
        glbFeature &feature = context.features[iFeature];
        if (feature.api == "wgl") {
            result = glbBuildGenerateCode_C_InitCurrentContextAPI_Feature(context, context.features[iFeature], processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_WGL */\n";

    // GLX features.
    codeOut += "#if defined(GLBIND_GLX)\n";
    for (size_t iFeature = 0; iFeature < context.features.size(); ++iFeature) {
        glbFeature &feature = context.features[iFeature];
        if (feature.api == "glx") {
            result = glbBuildGenerateCode_C_InitCurrentContextAPI_Feature(context, context.features[iFeature], processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_GLX */\n";


    // GL extensions.
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && (extension.supported == "gl" || glbContains(extension.supported, "gl|") || glbContains(extension.supported, "glcore"))) {
            result = glbBuildGenerateCode_C_InitCurrentContextAPI_Extension(context, extension, processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }

    // WGL extensions.
    codeOut += "#if defined(GLBIND_WGL)\n";
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && glbContains(extension.supported, "wgl")) {
            result = glbBuildGenerateCode_C_InitCurrentContextAPI_Extension(context, extension, processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_WGL */\n";

    // GLX extensions.
    codeOut += "#if defined(GLBIND_GLX)\n";
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && glbContains(extension.supported, "glx")) {
            result = glbBuildGenerateCode_C_InitCurrentContextAPI_Extension(context, extension, processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_GLX */";

    return GLB_SUCCESS;
}


glbResult glbBuildGenerateCode_C_SetGlobalAPIFromStruct_RequireCommands(glbBuild &context, const glbRequire &require, std::vector<std::string> &processedCommands, std::string &codeOut)
{
    glbResult result;

    for (size_t iCommand = 0; iCommand < require.commands.size(); ++iCommand) {
        std::string commandName = require.commands[iCommand];
        if (!glbBuildIsCommandIgnored(commandName.c_str()) && std::find(processedCommands.begin(), processedCommands.end(), commandName) == processedCommands.end()) {
            glbCommand* pCommand;
            result = glbBuildFindCommand(context, require.commands[iCommand].c_str(), &pCommand);
            if (result != GLB_SUCCESS) {
                return result;
            }

            codeOut += "    " + pCommand->name + " = pAPI->" + pCommand->name + ";\n";

            processedCommands.push_back(commandName);
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_SetGlobalAPIFromStruct_Feature(glbBuild &context, const glbFeature &feature, std::vector<std::string> &processedCommands, std::string &codeOut)
{
    for (size_t iRequire = 0; iRequire < feature.requires.size(); ++iRequire) {
        glbResult result = glbBuildGenerateCode_C_SetGlobalAPIFromStruct_RequireCommands(context, feature.requires[iRequire], processedCommands, codeOut);
        if (result != GLB_SUCCESS) {
            return result;
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_SetGlobalAPIFromStruct_Extension(glbBuild &context, const glbExtension &extension, std::vector<std::string> &processedCommands, std::string &codeOut)
{
    for (size_t iRequire = 0; iRequire < extension.requires.size(); ++iRequire) {
        glbResult result = glbBuildGenerateCode_C_SetGlobalAPIFromStruct_RequireCommands(context, extension.requires[iRequire], processedCommands, codeOut);
        if (result != GLB_SUCCESS) {
            return result;
        }
    }

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_SetGlobalAPIFromStruct(glbBuild &context, std::string &codeOut)
{
    glbResult result;
    std::vector<std::string> processedCommands;

    // GL features.
    for (size_t iFeature = 0; iFeature < context.features.size(); ++iFeature) {
        glbFeature &feature = context.features[iFeature];
        if (feature.api == "gl") {
            result = glbBuildGenerateCode_C_SetGlobalAPIFromStruct_Feature(context, context.features[iFeature], processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }

    // Some system APIs should not be bound to global scope because they declare their functions statically.
#if 0
    // WGL features.

    // GLX features.
#endif

    // GL extensions.
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && (extension.supported == "gl" || glbContains(extension.supported, "gl|") || glbContains(extension.supported, "glcore"))) {
            result = glbBuildGenerateCode_C_SetGlobalAPIFromStruct_Extension(context, extension, processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }

    // WGL extensions.
    codeOut += "#if defined(GLBIND_WGL)\n";
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && glbContains(extension.supported, "wgl")) {
            result = glbBuildGenerateCode_C_SetGlobalAPIFromStruct_Extension(context, extension, processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_WGL */\n";

    // GLX extensions.
    codeOut += "#if defined(GLBIND_GLX)\n";
    for (size_t iExtension = 0; iExtension < context.extensions.size(); ++iExtension) {
        glbExtension &extension = context.extensions[iExtension];
        if (!glbIsExtensionBlackListed(extension.name.c_str()) && glbContains(extension.supported, "glx")) {
            result = glbBuildGenerateCode_C_SetGlobalAPIFromStruct_Extension(context, extension, processedCommands, codeOut);
            if (result != GLB_SUCCESS) {
                return result;
            }
        }
    }
    codeOut += "#endif /* GLBIND_GLX */";

    return GLB_SUCCESS;
}

#include <time.h>
glbResult glbBuildGenerateCode_C_Date(glbBuild &context, std::string &codeOut)
{
    (void)context;

    time_t t = time(NULL);

    char dateStr[32];
#if defined(_MSC_VER)
    struct tm local;
    localtime_s(&local, &t);
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &local);
#else
    struct tm *local = localtime(&t);
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", local);
#endif

    codeOut += dateStr;
    return GLB_SUCCESS;
}

glbResult glbBuildGetOpenGLVersion(glbBuild &context, std::string &versionOut)
{
    // The version can be retrieved from the last "gl" feature section.
    std::string versionStr;
    for (auto feature : context.features) {
        if (feature.api == "gl") {
            versionStr = feature.number;
        }
    }

    versionOut = versionStr;

    return GLB_SUCCESS;
}

glbResult vkbBuildGenerateCode_C_OpenGLVersion(glbBuild &context, std::string &codeOut)
{
    std::string version;
    glbResult result = glbBuildGetOpenGLVersion(context, version);
    if (result != GLB_SUCCESS) {
        return result;
    }

    codeOut += version;

    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C_Revision(glbBuild &context, std::string &codeOut)
{
    // Rules for the revision number:
    // 1) If the OpenGL version has changed, reset the revision to 0, otherwise increment by 1.
    // 2) If glbind.h cannot be found, set to 0.
    std::string revision;

    size_t fileSize;
    char* pFileData;
    if (glbOpenAndReadTextFile("../../glbind.h", &fileSize, &pFileData) == GLB_SUCCESS) {
        // We need to parse the previous version.
        std::string prevVersionMajor;
        std::string prevVersionMinor;
        std::string prevVersionRevision;
        const char* versionBeg = strstr((const char*)pFileData, "glbind - v");
        if (versionBeg != NULL) {
            versionBeg += strlen("glbind - v");

            std::string currentOpenGLVersion;
            glbResult result = glbBuildGetOpenGLVersion(context, currentOpenGLVersion);
            if (result != GLB_SUCCESS) {
                return result;
            }

            const char* cursorBeg = versionBeg;
            const char* cursorEnd = versionBeg;

            cursorEnd = strstr(cursorBeg, ".");
            prevVersionMajor = std::string(cursorBeg, cursorEnd-cursorBeg);
            cursorBeg = cursorEnd + 1;

            cursorEnd = strstr(cursorBeg, ".");
            prevVersionMinor = std::string(cursorBeg, cursorEnd-cursorBeg);
            cursorBeg = cursorEnd + 1;

            cursorEnd = strstr(cursorBeg, " ");
            prevVersionRevision = std::string(cursorBeg, cursorEnd-cursorBeg);
            cursorBeg = cursorEnd + 1;

            std::string previousOpenGLVersion = prevVersionMajor + "." + prevVersionMinor;
            if (currentOpenGLVersion == previousOpenGLVersion) {
                // OpenGL versions are the same, so increment.
                revision = std::to_string(atoi(prevVersionRevision.c_str()) + 1);
            } else {
                // OpenGL versions are different, so reset.
                revision = "0";
            }
        } else {
            // Couldn't find the previous version.
            revision = "0";
        }
    } else {
        // vkbind.h was not found.
        revision = "0";
    }

    codeOut += revision;
    return GLB_SUCCESS;
}

glbResult glbBuildGenerateCode_C(glbBuild &context, const char* tag, std::string &codeOut)
{
    if (tag == NULL) {
        return GLB_INVALID_ARGS;
    }

    glbResult result = GLB_INVALID_ARGS;
    if (strcmp(tag, "/*<<opengl_main>>*/") == 0) {
        result = glbBuildGenerateCode_C_Main(context, codeOut);
    }
    if (strcmp(tag, "/*<<opengl_funcpointers_decl_global>>*/") == 0) {
        result = glbBuildGenerateCode_C_FuncPointersDeclGlobal(context, 0, codeOut);
    }
    if (strcmp(tag, "/*<<opengl_funcpointers_decl_global:4>>*/") == 0) {
        result = glbBuildGenerateCode_C_FuncPointersDeclGlobal(context, 4, codeOut);
    }
    if (strcmp(tag, "/*<<init_current_context_api>>*/") == 0) {
        result = glbBuildGenerateCode_C_InitCurrentContextAPI(context, codeOut);
    }
    if (strcmp(tag, "/*<<set_global_api_from_struct>>*/") == 0) {
        result = glbBuildGenerateCode_C_SetGlobalAPIFromStruct(context, codeOut);
    }
    if (strcmp(tag, "<<opengl_version>>") == 0) {
        result = vkbBuildGenerateCode_C_OpenGLVersion(context, codeOut);
    }
    if (strcmp(tag, "<<revision>>") == 0) {
        result = glbBuildGenerateCode_C_Revision(context, codeOut);
    }
    if (strcmp(tag, "<<date>>") == 0) {
        result = glbBuildGenerateCode_C_Date(context, codeOut);
    }

    return result;
}

glbResult glbBuildGenerateOutputFile(glbBuild &context, const char* outputFilePath)
{
    // Before doing anything we need to grab the template.
    size_t templateFileSize;
    char* pTemplateFileData;
    glbResult result = glbOpenAndReadTextFile(GLB_BUILD_TEMPLATE_PATH, &templateFileSize, &pTemplateFileData);
    if (result != GLB_SUCCESS) {
        return result;
    }

    std::string outputStr = pTemplateFileData;
    free(pTemplateFileData);

    // There will be a series of tags that we need to replace with generated code.
    const char* tags[] = {
        "/*<<opengl_main>>*/",
        "/*<<opengl_funcpointers_decl_global>>*/",
        "/*<<opengl_funcpointers_decl_global:4>>*/",
        "/*<<init_current_context_api>>*/",
        "/*<<set_global_api_from_struct>>*/",
        "<<opengl_version>>",
        "<<revision>>",
        "<<date>>",
    };

    for (size_t iTag = 0; iTag < sizeof(tags)/sizeof(tags[0]); ++iTag) {
        std::string generatedCode;
        result = glbBuildGenerateCode_C(context, tags[iTag], generatedCode);
        if (result != GLB_SUCCESS) {
            return result;
        }

        glbReplaceAllInline(outputStr, tags[iTag], generatedCode);
    }

    glbOpenAndWriteTextFile(outputFilePath, outputStr.c_str());
    return GLB_SUCCESS;
}


int main(int argc, char** argv)
{
    glbBuild context;
    glbResult result;

    // GL
    result = glbBuildLoadXMLFile(context, GLB_BUILD_XML_PATH_GL);
    if (result != GLB_SUCCESS) {
        return result;
    }
    
    // WGL
    result = glbBuildLoadXMLFile(context, GLB_BUILD_XML_PATH_WGL);
    if (result != GLB_SUCCESS) {
        return result;
    }

    // GLX
    result = glbBuildLoadXMLFile(context, GLB_BUILD_XML_PATH_GLX);
    if (result != GLB_SUCCESS) {
        return result;
    }


    // Debugging
#if 0
    for (size_t i = 0; i < context.types.size(); ++i) {
        printf("<type>: name=%s, valueC=%s, requires=%s\n", context.types[i].name.c_str(), context.types[i].valueC.c_str(), context.types[i].requires.c_str());
    }

    for (size_t i = 0; i < context.enums.size(); ++i) {
        printf("<enums>\n");
        for (size_t j = 0; j < context.enums[i].enums.size(); ++j) {
            printf("  <enum name=\"%s\" value=\"%s\">\n", context.enums[i].enums[j].name.c_str(), context.enums[i].enums[j].value.c_str());
        }
        printf("</enums>\n");
    }

    for (size_t i = 0; i < context.groups.size(); ++i) {
        printf("<group name\"%s\">\n", context.groups[i].name.c_str());
        for (size_t j = 0; j < context.groups[i].enums.size(); ++j) {
            printf("  <enum name=\"%s\" value=\"%s\">\n", context.groups[i].enums[j].name.c_str(), context.groups[i].enums[j].value.c_str());
        }
        printf("</group>\n");
    }

    for (size_t i = 0; i < context.commands.size(); ++i) {
        printf("<commands namespace\"%s\">\n", context.commands[i].namespaceAttrib.c_str());
        for (size_t j = 0; j < context.commands[i].commands.size(); ++j) {
            printf("  %s %s()\n", context.commands[i].commands[j].returnTypeC.c_str(), context.commands[i].commands[j].name.c_str());
        }
        printf("</commands>\n");
    }
#endif


    // Output file.
    result = glbBuildGenerateOutputFile(context, "../../glbind.h");
    if (result != GLB_SUCCESS) {
        printf("Failed to generate output file.\n");
        return (int)result;
    }

    // Getting here means we're done.
    (void)argc;
    (void)argv;
    return 0;
}