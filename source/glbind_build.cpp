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

typedef int glbResult;
#define GLB_SUCCESS                 0
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


struct glbBuild
{
    std::vector<glbType>     types;
    std::vector<glbGroup>    groups;
    std::vector<glbEnums>    enums;
    std::vector<glbCommands> commands;
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
#if 1
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


    // TODO: Output file.


    // Getting here means we're done.
    (void)argc;
    (void)argv;
    return 0;
}