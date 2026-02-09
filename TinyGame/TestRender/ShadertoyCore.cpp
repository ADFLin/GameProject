#include "ShadertoyCore.h"

#include "RHI/DrawUtility.h"
#include "RHI/ShaderManager.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIUtility.h"

#include "Serialize/FileStream.h"

#include "RenderDebug.h"
#include "Misc/Format.h"
#include <regex>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <iostream>
#include <cctype>

#include "RHI/RHICommand.h"



namespace Shadertoy
{
	IMPLEMENT_SHADER(CubeOnePassVS, EShader::Vertex, SHADER_ENTRY(CubeOnePassVS));

	class GLSLToHLSLConverter
	{
	public:
		struct Token
		{
			enum EType { None, Identifier, Keyword, Number, Operator, Punctuation, String, Preprocessor, Comment, Whitespace };
			EType type = None;
			std::string text;
			std::string outText;
			size_t line = 0;
			bool isMatrix = false;
			bool isVector = false;
		};

		static bool IsAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
		static bool IsDigit(char c) { return c >= '0' && c <= '9'; }
		static bool IsAlphaNumeric(char c) { return IsAlpha(c) || IsDigit(c); }

		std::vector<Token> tokens;
		std::string result;

		void tokenize(std::string const& source)
		{
			tokens.clear();
			size_t i = 0;
			size_t line = 1;
			while (i < source.size())
			{
				char c = source[i];
				Token tk;
				tk.line = line;

				if (std::isspace(static_cast<unsigned char>(c)))
				{
					tk.type = Token::Whitespace;
					while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])))
					{
						if (source[i] == '\n')
						{
							line++;
						}
						tk.text += source[i++];
					}
				}
				else if (c == '/' && i + 1 < source.size() && (source[i + 1] == '/' || source[i + 1] == '*'))
				{
					tk.type = Token::Comment;
					if (source[i + 1] == '/') // Single line
					{
						while (i < source.size() && source[i] != '\n')
						{
							tk.text += source[i++];
						}
					}
					else // Block comment
					{
						tk.text += "/*"; i += 2;
						while (i + 1 < source.size() && !(source[i] == '*' && source[i + 1] == '/'))
						{
							if (source[i] == '\n')
							{
								line++;
							}
							tk.text += source[i++];
						}
						tk.text += "*/"; i += 2;
					}
				}
				else if (c == '#')
				{
					tk.type = Token::Preprocessor;
					while (i < source.size() && source[i] != '\n')
					{
						tk.text += source[i++];
					}
				}
				else if (c == '"')
				{
					tk.type = Token::String;
					tk.text += c; i++;
					while (i < source.size() && source[i] != '"')
					{
						if (source[i] == '\\')
						{
							tk.text += source[i++];
						}
						tk.text += source[i++];
					}
					if (i < source.size()) tk.text += source[i++];
				}
				else if (IsAlpha(c))
				{
					while (i < source.size() && IsAlphaNumeric(source[i]))
					{
						tk.text += source[i++];
					}
					tk.type = Token::Identifier;
				}
				else if (IsDigit(c) || (c == '.' && i + 1 < source.size() && IsDigit(source[i + 1])))
				{
					tk.type = Token::Number;
					while (i < source.size() && (IsAlphaNumeric(source[i]) || source[i] == '.'))
					{
						tk.text += source[i++];
					}
				}
				else
				{
					if (std::string("(){}[],;").find(c) != std::string::npos)
					{
						tk.type = Token::Punctuation;
						tk.text += c; i++;
					}
					else
					{
						tk.type = Token::Operator;
						while (i < source.size() && std::string("(){}[],;\"_").find(source[i]) == std::string::npos && !IsAlphaNumeric(source[i]) && !std::isspace(static_cast<unsigned char>(source[i])))
						{
							tk.text += source[i++];
						}
					}
				}

				tk.outText = tk.text;
				tokens.push_back(tk);
			}
		}

		struct SymbolInfo
		{
			std::string type;
			bool isMatrix = false;
			bool isVector = false;
			bool isScalar = false;
		};

		std::vector<std::map<std::string, SymbolInfo>> scopes;
		int scopeDepth = 0;
		int parenDepth = 0;
		std::set<int> prefixedIndices;

		void pushScope() { scopes.push_back({}); scopeDepth++; }
		void popScope()
		{
			if (!scopes.empty())
			{
				scopes.pop_back();
			}
			scopeDepth--;
		}
		std::map<std::string, SymbolInfo> persistentSymbols;
		void addSymbol(std::string const& name, SymbolInfo info)
		{
			if (!scopes.empty()) scopes.back()[name] = info;
			persistentSymbols[name] = info;
		}

		SymbolInfo* findSymbol(std::string const& name)
		{
			for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
			{
				auto sit = it->find(name);
				if (sit != it->end()) return &sit->second;
			}
			return nullptr;
		}

		std::string resolveType(std::string const& t)
		{
			if (t == "uint")
			{
				return "uint";
			}
			// vec and mat handling moved to Parse
			// Matrix handling moved to Parse
			if (t == "mix")
			{
				return "lerp";
			}
			if (t == "fract")
			{
				return "frac";
			}
			if (t == "sampler1D")
			{
				return "Texture1D";
			}
			if (t == "sampler2D")
			{
				return "Texture2D";
			}
			if (t == "sampler3D")
			{
				return "Texture3D";
			}
			if (t == "samplerCube")
			{
				return "TextureCube";
			}
			return t;
		}
		

		bool isVector(std::string const& t)
		{
			if (t.find("vec") != std::string::npos) return true;
			if (t.size() >= 5 && t.find("float") == 0 && std::isdigit(t[5])) return true;
			if (t.size() >= 3 && t.find("int") == 0 && std::isdigit(t[3])) return true;
			if (t.size() >= 4 && t.find("uint") == 0 && std::isdigit(t[4])) return true;
			return t.find("GLSLVec") == 0 || t.find("GLSLIVec") == 0;
		}

		bool isMatrix(std::string const& t)
		{
			if (t.find("mat") != std::string::npos) return true;
			if (t.find("GLSLMat") == 0) return true;
			if (t.size() >= 8 && t.find("float") == 0 && std::isdigit(t[5]) && t[6] == 'x' && std::isdigit(t[7])) return true;
			return false;
		}

		bool isType(std::string const& t)
		{
			static const std::set<std::string> types = { 
				"void", "bool", "int", "uint", "float", "double", 
				"vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "uvec2", "uvec3", "uvec4",
				"mat2", "mat3", "mat4", "mat2x2", "mat3x3", "mat4x4",
				"float2", "float3", "float4", "int2", "int3", "int4", "uint2", "uint3", "uint4",
				"float2x2", "float3x3", "float4x4", "float4x3", "float3x4", "float2x3", "float3x2"
			};
			if (types.count(t) > 0) return true;
			if (t.find("GLSL") == 0 || t.find("Texture") == 0 || t.find("RWTexture") == 0 || t.find("SamplerState") == 0 || t.find("Texture2D") == 0) return true;
			return false;
		}

		bool isQualifier(std::string const& t)
		{
			static const std::set<std::string> qualifiers = { 
				"const", "uniform", "in", "out", "varying", "attribute", "static", "coherent", "readonly", "writeonly", "layout", "precision", "highp", "mediump", "lowp", "flat", "smooth", "noperspective",
				"VS_INPUT", "VS_OUTPUT", "PS_INPUT", "PS_OUTPUT", "VS_INPUT_INSTANCEID", "VS_OUTPUT_RENDERTARGET_INDEX", "PS_INPUT_RENDERTARGET_INDEX"
			};
			return qualifiers.count(t) > 0;
		}

		void parse()
		{
			scopes.clear();
			pushScope();
			parenDepth = 0;

			for (size_t i = 0; i < tokens.size(); ++i)
			{
				Token& t = tokens[i];
				std::string resolvedType = resolveType(t.text);
				if (resolvedType != t.text)
				{
					t.outText = resolvedType;
				}

				// Special handling for Matrix and Vector Types vs Constructors
				if (t.text.find("mat") == 0 || t.text.find("vec") == 0 || t.text.find("ivec") == 0 ||
					t.text.find("float") == 0 || t.text.find("int") == 0 || t.text.find("uint") == 0)
				{
					int nextIdx = getNextNonWhitespace(i);
					bool isConstructor = (nextIdx != -1 && tokens[nextIdx].text == "(");

					if (t.text.find("mat") == 0 || (t.text.find("float") == 0 && t.text.find('x') != std::string::npos))
					{
						std::string suffix = "";
						if (t.text.find("mat") == 0)
						{
							if (t.text.size() > 3) suffix = t.text.substr(3);
							if (suffix == "2x2") suffix = "2";
							else if (suffix == "3x3") suffix = "3";
							else if (suffix == "4x4") suffix = "4";
						}
						else
						{
							size_t xPos = t.text.find('x');
							if (xPos != std::string::npos && xPos > 5)
								suffix = t.text.substr(5, xPos - 5);
							else if (t.text.size() > 5)
								suffix = t.text.substr(5);
						}

						if (isConstructor)
						{
							t.outText = "GLSLMat" + suffix;
							t.isMatrix = true;
						}
						else
						{
							t.outText = "float" + suffix + "x" + suffix;
							t.isMatrix = true;
						}
					}
					else if (t.text.find("vec") != std::string::npos || 
						     (t.text.size() > 5 && t.text.find("float") == 0 && std::isdigit(t.text[5])) ||
						     (t.text.size() > 3 && t.text.find("int") == 0 && std::isdigit(t.text[3])) ||
						     (t.text.size() > 4 && t.text.find("uint") == 0 && std::isdigit(t.text[4])))
					{
						std::string prefix;
						std::string suffix;

						if (t.text.find("vec") != std::string::npos)
						{
							prefix = (t.text.find("ivec") == 0) ? "int" : "float";
							suffix = t.text.substr(t.text.find("vec") + 3);
						}
						else
						{
							if (t.text.find("float") == 0) { prefix = "float"; suffix = t.text.substr(5); }
							else if (t.text.find("int") == 0) { prefix = "int"; suffix = t.text.substr(3); }
							else if (t.text.find("uint") == 0) { prefix = "uint"; suffix = t.text.substr(4); }
						}

						if (isConstructor)
						{
							t.outText = (prefix == "int" || prefix == "uint" ? "GLSLIVec" : "GLSLVec") + suffix;
							t.isVector = true;
						}
						else
						{
							t.outText = prefix + suffix;
							t.isVector = true;
						}
					}

					// Combined Array Constructor Check (Type[N](...) -> { ... })
					if (t.isMatrix || t.isVector || isType(t.outText) || isType(t.text))
					{
						int arrayIdx = getNextNonWhitespace(i);
						if (arrayIdx != -1 && tokens[arrayIdx].text == "[")
						{
							int closeIdx = getNextNonWhitespace(arrayIdx);
							int balance = 1;
							while (closeIdx != -1)
							{
								if (tokens[closeIdx].text == "[") balance++;
								else if (tokens[closeIdx].text == "]") balance--;
								if (balance == 0) break;
								closeIdx = getNextNonWhitespace(closeIdx);
							}

							if (closeIdx != -1)
							{
								int parenIdx = getNextNonWhitespace(closeIdx);
								if (parenIdx != -1 && tokens[parenIdx].text == "(")
								{
									// Convert array constructor to initializer list: Type[N](...) -> { ... }
									t.outText = ""; 
									for (int k = arrayIdx; k <= closeIdx; ++k) tokens[k].outText = "";

									tokens[parenIdx].outText = "{";
									int pBalance = 1;
									int j = parenIdx + 1;
									while (j < (int)tokens.size())
									{
										if (tokens[j].text == "(") pBalance++;
										else if (tokens[j].text == ")") pBalance--;
										if (pBalance == 0) { tokens[j].outText = "}"; break; }
										j++;
									}
								}
							}
						}
					}
				}

				if (t.type == Token::Punctuation)
				{
					if (t.text == "{")
					{
						pushScope();
					}
					else if (t.text == "}")
					{
						popScope();
					}
					else if (t.text == "(")
					{
						parenDepth++;
					}
					else if (t.text == ")")
					{
						parenDepth--;
					}
					else if (t.text == ".")
					{
						int prevIdx = getPrevNonWhitespace(i);
						int nextIdx = getNextNonWhitespace(i);
						if (prevIdx != -1 && nextIdx != -1)
						{
							Token& prev = tokens[prevIdx];
							Token& next = tokens[nextIdx];
							if (prev.isVector || isVector(prev.outText))
							{
								bool hasSTPQ = false;
								for (char c : next.text) { if (c == 's' || c == 't' || c == 'p' || c == 'q') { hasSTPQ = true; break; } }
								if (hasSTPQ)
								{
									std::string newSwizzle = "";
									for (char c : next.text)
									{
										if (c == 's') newSwizzle += 'x';
										else if (c == 't') newSwizzle += 'y';
										else if (c == 'p') newSwizzle += 'z';
										else if (c == 'q') newSwizzle += 'w';
										else newSwizzle += c;
									}
									next.outText = newSwizzle;
								}
							}
						}
					}
				}
				else if (t.type == Token::Identifier)
				{
					if (t.text == "for")
					{
						t.outText = "[loop] for";
					}

					int nextIdx = getNextNonWhitespace(i);
					if (nextIdx != -1 && tokens[nextIdx].type == Token::Identifier)
					{
						int afterNameIdx = getNextNonWhitespace(nextIdx);
						bool isFunc = (afterNameIdx != -1 && tokens[afterNameIdx].text == "(");

						if (!isFunc)
						{
							std::string varType = t.outText.empty() ? t.text : t.outText;
							std::string varName = tokens[nextIdx].text;
							int varTypeIdx = i;

							if (isQualifier(varType))
							{
								int typeIdx = getNextNonWhitespace(nextIdx);
								if (typeIdx != -1 && tokens[typeIdx].type == Token::Identifier)
								{
									varType = tokens[nextIdx].outText.empty() ? tokens[nextIdx].text : tokens[nextIdx].outText;
									varName = tokens[typeIdx].text;
									varTypeIdx = nextIdx;
								}
							}

							bool handledByQualifier = false;
							int prevQualIdx = getPrevNonWhitespace(i);
							if (prevQualIdx != -1 && isQualifier(tokens[prevQualIdx].text))
							{
								handledByQualifier = true;
							}

							if (!handledByQualifier)
							{
								if (!isQualifier(varType) && !isType(varName))
								{
									SymbolInfo info;
									info.type = varType;
									info.isMatrix = isMatrix(varType);
									info.isVector = isVector(varType);
									addSymbol(varName, info);
								}

								if (scopeDepth == 1 && parenDepth == 0)
								{
									if (varType != "cbuffer" && prefixedIndices.count(i) == 0)
									{
										auto hasQualifierInStr = [](std::string const& s) {
											if (s.empty()) return false;
											return s.find("static") != std::string::npos || s.find("uniform") != std::string::npos || 
												   s.find("in ") == 0 || s.find("out ") == 0 || s.find("varying") != std::string::npos;
										};

										bool alreadyQualified = false;
										for (int k = i; k <= varTypeIdx; ++k)
										{
											if (hasQualifierInStr(tokens[k].text) || hasQualifierInStr(tokens[k].outText))
											{
												alreadyQualified = true;
												break;
											}
										}
										
										if (!alreadyQualified)
										{
											int pIdx = getPrevNonWhitespace(i);
											while (pIdx != -1)
											{
												std::string const& pText = tokens[pIdx].text;
												if (pText == "static" || pText == "uniform" || pText == "in" || pText == "out" || pText == "varying" || pText == "attribute")
												{
													alreadyQualified = true;
													break;
												}
												if (pText == ";" || pText == "{" || pText == "}" || pText == "(" || pText == ")") break;
												pIdx = getPrevNonWhitespace(pIdx);
											}
										}

										if (!alreadyQualified && isType(varType) && varName.find("VS_") != 0 && varName.find("PS_") != 0)
										{
											tokens[i].outText = "static " + (tokens[i].outText.empty() ? tokens[i].text : tokens[i].outText);
											prefixedIndices.insert(i);
											if (afterNameIdx != -1 && tokens[afterNameIdx].text == ";")
											{
												tokens[nextIdx].outText += " = (" + varType + ")0";
											}
										}
									}
								}
							}

							if (isVector(varType) && afterNameIdx != -1 && tokens[afterNameIdx].text == "=")
							{
								int rhsIdx = getNextNonWhitespace(afterNameIdx);
								if (rhsIdx != -1)
								{
									Token& rhs = tokens[rhsIdx];
									int afterRhs = getNextNonWhitespace(rhsIdx);
									if (afterRhs != -1 && (tokens[afterRhs].text == ";" || tokens[afterRhs].text == ","))
									{
										rhs.outText = "(" + varType + ")(" + rhs.outText + ")";
									}
								}
							}
						}
						else
						{
							// Function declaration - register parameters for the NEXT scope
							int pIdx = getNextNonWhitespace(afterNameIdx); // after "("
							while (pIdx != -1 && tokens[pIdx].text != ")")
							{
								if (tokens[pIdx].type == Token::Identifier)
								{
									int nIdx = getNextNonWhitespace(pIdx);
									if (nIdx != -1 && tokens[nIdx].type == Token::Identifier)
									{
										std::string pType = tokens[pIdx].outText.empty() ? tokens[pIdx].text : tokens[pIdx].outText;
										std::string pName = tokens[nIdx].text;
										if (isQualifier(pType))
										{
											int nnIdx = getNextNonWhitespace(nIdx);
											if (nnIdx != -1 && tokens[nnIdx].type == Token::Identifier)
											{
												pType = tokens[nIdx].outText.empty() ? tokens[nIdx].text : tokens[nIdx].outText;
												pName = tokens[nnIdx].text;
												pIdx = nIdx;
												nIdx = nnIdx;
											}
										}
										SymbolInfo info;
										info.type = pType;
										info.isMatrix = isMatrix(pType);
										info.isVector = isVector(pType);
										addSymbol(pName, info);
										pIdx = nIdx;
									}
								}
								pIdx = getNextNonWhitespace(pIdx);
							}

							SymbolInfo info;
							info.type = t.outText.empty() ? t.text : t.outText;
							info.isMatrix = isMatrix(info.type);
							info.isVector = isVector(info.type);
							addSymbol(tokens[nextIdx].text, info);
						}
					}
					else
					{
						if (t.outText == "texelFetch")
						{
							t.outText = "GLSLTexelFetch";
						}
					}

					SymbolInfo* info = findSymbol(t.text);
					if (info)
					{
						t.isMatrix = info->isMatrix;
						t.isVector = info->isVector;
					}
				}
			}
			fixMatrixMultiplication();
		}

		void fixMatrixMultiplication()
		{
			enum EOpType { OpNone, OpMatrix, OpVector };
			auto getOpType = [&](int idx)
			{
				if (idx < 0 || idx >= (int)tokens.size())
				{
					return OpNone;
				}
				Token& tk = tokens[idx];
				if (tk.isMatrix || isMatrix(tk.outText) || tk.outText.find("GLSLMat") == 0)
				{
					return OpMatrix;
				}
				if (tk.isVector || isVector(tk.outText) || tk.outText.find("GLSLVec") == 0)
				{
					return OpVector;
				}
				if (tk.outText == "mul(")
				{
					return OpMatrix;
				}

				static const std::set<std::string> vecFuncs = { "normalize", "reflect", "refract", "cross", "abs", "cos", "sin", "tan", "frac", "sqrt", "pow", "mod", "min", "max", "mix", "clamp", "floor", "ceil", "step", "smoothstep" };
				if (vecFuncs.count(tk.outText))
				{
					return OpVector;
				}

				if (tk.type == Token::Identifier)
				{
					if (isMatrix(tk.text)) return OpMatrix;
					if (isVector(tk.text)) return OpVector;
					
					// Check converted text
					if (isMatrix(tk.outText)) return OpMatrix;
					if (isVector(tk.outText)) return OpVector;

					// Symbols might have been cleared, but Token flags should be set
					if (tk.isMatrix) return OpMatrix;
					if (tk.isVector) return OpVector;

					// Double check with type name patterns
					if (isMatrix(tk.outText) || isMatrix(tk.text)) return OpMatrix;
					if (isVector(tk.outText) || isVector(tk.text)) return OpVector;

					// Last resort: check persistent symbols
					auto it = persistentSymbols.find(tk.text);
					if (it != persistentSymbols.end())
					{
						if (it->second.isMatrix) return OpMatrix;
						if (it->second.isVector) return OpVector;
					}
				}
				return OpNone;
			};

			auto findLHSStart = [&](int end)
			{
				int curr = end;
				while (curr >= 0)
				{
					int prev = getPrevNonWhitespace(curr);
					if (tokens[curr].text == ")")
					{
						int balance = 1;
						int j = curr - 1;
						while (j >= 0)
						{
							if (tokens[j].text == ")") balance++;
							else if (tokens[j].text == "(") balance--;
							if (balance == 0) break;
							j--;
						}
						if (j >= 0)
						{
							curr = j;
							int nameIdx = getPrevNonWhitespace(j);
							if (nameIdx != -1 && (tokens[nameIdx].type == Token::Identifier || tokens[nameIdx].text == "mul(" || tokens[nameIdx].outText.find("mul(") == 0)) curr = nameIdx;
						}
						else break;
					}
					else if (tokens[curr].text == "]")
					{
						int balance = 1;
						int j = curr - 1;
						while (j >= 0)
						{
							if (tokens[j].text == "]") balance++;
							else if (tokens[j].text == "[") balance--;
							if (balance == 0) break;
							j--;
						}
						if (j >= 0)
						{
							curr = j;
						}
						else
						{
							break;
						}
					}
					else if (tokens[curr].type == Token::Identifier || tokens[curr].type == Token::Number)
					{
						if (prev != -1 && tokens[prev].text == ".")
						{
							int base = getPrevNonWhitespace(prev);
							if (base != -1) { curr = base; continue; }
						}
						break;
					}
					else break;
				}
				return curr;
			};

			auto findRHSEnd = [&](int start)
			{
				int curr = start;
				while (curr < (int)tokens.size())
				{
					int next = getNextNonWhitespace(curr);
					if (next != -1 && tokens[next].text == "(")
					{
						int balance = 1;
						int j = next + 1;
						while (j < (int)tokens.size())
						{
							if (tokens[j].text == "(")
							{
								balance++;
							}
							else if (tokens[j].text == ")")
							{
								balance--;
							}
							if (balance == 0)
							{
								break;
							}
							j++;
						}
						if (j < (int)tokens.size())
						{
							curr = j;
						}
						else
						{
							break;
						}
					}
					else if (next != -1 && tokens[next].text == "[")
					{
						int balance = 1;
						int j = next + 1;
						while (j < (int)tokens.size())
						{
							if (tokens[j].text == "[")
							{
								balance++;
							}
							else if (tokens[j].text == "]")
							{
								balance--;
							}
							if (balance == 0)
							{
								break;
							}
							j++;
						}
						if (j < (int)tokens.size())
						{
							curr = j;
						}
						else
						{
							break;
						}
					}
					else
					{
						if (next != -1 && tokens[next].text == ".")
						{
							int sub = getNextNonWhitespace(next);
							if (sub != -1 && tokens[sub].type == Token::Identifier)
							{
								curr = sub;
								continue;
							}
						}
						break;
					}
				}
				return curr;
			};

			bool changed = true;
			while (changed)
			{
				changed = false;
				for (size_t i = 1; i < tokens.size() - 1; ++i)
				{
					Token& t = tokens[i];
					bool isMulAssign = (t.type == Token::Operator && t.text == "*=");
					if (t.type == Token::Operator && (t.text == "*" || isMulAssign) && t.outText == t.text)
					{
						int lhsEnd = getPrevNonWhitespace(i);
						if (lhsEnd == -1) continue;
						int lhsStart = findLHSStart(lhsEnd);
						EOpType lhsType = getOpType(lhsStart);

						int rhsStart = getNextNonWhitespace(i);
						if (rhsStart == -1) continue;
						int rhsEnd = findRHSEnd(rhsStart);
						EOpType rhsType = getOpType(rhsStart);

						if ((lhsType == OpMatrix && (rhsType == OpMatrix || rhsType == OpVector)) ||
							(rhsType == OpMatrix && (lhsType == OpMatrix || lhsType == OpVector)))
						{
							if (isMulAssign)
							{
								std::string lhsStr = "";
								for (int k = lhsStart; k <= lhsEnd; ++k) lhsStr += (tokens[k].outText.empty() ? tokens[k].text : tokens[k].outText);
								t.outText = "= mul(" + lhsStr + ", ";
								tokens[rhsEnd].outText += ")";
							}
							else
							{
								tokens[lhsStart].outText = "mul(" + (tokens[lhsStart].outText.empty() ? tokens[lhsStart].text : tokens[lhsStart].outText);
								t.outText = ",";
								tokens[rhsEnd].outText += ")";
							}
							i = rhsEnd;
							changed = true;
						}
					}
				}
			}
		}

		int getPrevNonWhitespace(size_t start)
		{
			for (int k = (int)start - 1; k >= 0; --k)
			{
				if (k < (int)tokens.size() && tokens[k].type != Token::Whitespace && tokens[k].type != Token::Comment) return k;
			}
			return -1;
		}

		int getNextNonWhitespace(size_t start)
		{
			for (size_t k = start + 1; k < tokens.size(); ++k)
			{
				if (tokens[k].type != Token::Whitespace && tokens[k].type != Token::Comment) return (int)k;
			}
			return -1;
		}

		std::string getResult()
		{
			std::string res;
			for (auto const& t : tokens) res += t.outText;
			return res;
		}

		static std::string Run(std::string const& glsl)
		{
			if (glsl.empty())
			{
				return "";
			}
			GLSLToHLSLConverter parser;
			parser.tokenize(glsl);
			parser.parse();

			std::string code = parser.getResult();

			code = std::regex_replace(code, std::regex("\\blayout\\s*\\([^)]+\\)\\s*"), "");
			code = std::regex_replace(code, std::regex("\\buniform\\s+Texture(1D|2D|3D|Cube)\\b\\s+(\\w+)\\s*;"), "Texture$1<float4> $2; SamplerState $2Sampler;");
			code = std::regex_replace(code, std::regex("\\btexture(?:1D|2D|3D|Cube)?\\s*\\(\\s*(\\w+)\\s*,"), "GLSLTexture($1, $1Sampler, ");
			code = std::regex_replace(code, std::regex("\\btexture(?:1D|2D|3D|Cube)?Lod\\s*\\(\\s*(\\w+)\\s*,"), "GLSLTextureLod($1, $1Sampler, ");

			code = std::regex_replace(code, std::regex("\\bvec([234])\\b"), "float$1");
			code = std::regex_replace(code, std::regex("\\bivec([234])\\b"), "int$1");
			code = std::regex_replace(code, std::regex("\\bmat([234])\\b"), "float$1x$1");
			code = std::regex_replace(code, std::regex("\\bmix\\b"), "lerp");
			code = std::regex_replace(code, std::regex("\\bfract\\b"), "frac");
			code = std::regex_replace(code, std::regex("\\binverse\\b"), "GLSLInverse");
			code = std::regex_replace(code, std::regex("\\bnew\\b"), "_new");
			
			code = std::regex_replace(code, std::regex("\\bIDNEX\\b"), "INDEX");

			return code;
		}

		static std::string GetHelperCode()
		{
			return R"(
float mod(float x, float y) { return x - y * floor(x / y); }
float2 mod(float2 x, float2 y) { return x - y * floor(x / y); }
float3 mod(float3 x, float3 y) { return x - y * floor(x / y); }
float4 mod(float4 x, float4 y) { return x - y * floor(x / y); }

float2 mod(float2 x, float y) { return x - y * floor(x / y); }
float3 mod(float3 x, float y) { return x - y * floor(x / y); }
float4 mod(float4 x, float y) { return x - y * floor(x / y); }

float atan_wrap(float y, float x) { return atan2(y, x); }
float atan_wrap(float x) { return atan(x); }
#define atan atan_wrap

float4 GLSLTexelFetch(Texture1D<float4> tex, int p, int lod) { return tex.Load(int2(p, lod)); }
float4 GLSLTexelFetch(Texture2D<float4> tex, int2 p, int lod) { 
    uint w, h; tex.GetDimensions(w, h);
    return tex.Load(int3(p.x, (int)h - 1 - p.y, lod)); 
}
float4 GLSLTexelFetch(Texture3D<float4> tex, int3 p, int lod) { return tex.Load(int4(p, lod)); }

float4 GLSLTexture(Texture1D<float4> tex, SamplerState samp, float uv) { return tex.SampleLevel(samp, uv, 0); }
float4 GLSLTexture(Texture2D<float4> tex, SamplerState samp, float2 uv) { return tex.SampleLevel(samp, float2(uv.x, 1.0 - uv.y), 0); }
float4 GLSLTexture(Texture3D<float4> tex, SamplerState samp, float3 uv) { return tex.SampleLevel(samp, uv, 0); }
float4 GLSLTexture(TextureCube<float4> tex, SamplerState samp, float3 uv) { return tex.SampleLevel(samp, uv, 0); }

float4 GLSLTextureLod(Texture1D<float4> tex, SamplerState samp, float uv, float lod) { return tex.SampleLevel(samp, uv, lod); }
float4 GLSLTextureLod(Texture2D<float4> tex, SamplerState samp, float2 uv, float lod) { return tex.SampleLevel(samp, float2(uv.x, 1.0 - uv.y), lod); }
float4 GLSLTextureLod(Texture3D<float4> tex, SamplerState samp, float3 uv, float lod) { return tex.SampleLevel(samp, uv, lod); }
float4 GLSLTextureLod(TextureCube<float4> tex, SamplerState samp, float3 uv, float lod) { return tex.SampleLevel(samp, uv, lod); }

// Vector Helpers
float2 GLSLVec2(float x, float y) { return float2(x, y); }
float2 GLSLVec2(float2 x) { return x; }
float2 GLSLVec2(float x) { return float2(x, x); }
float3 GLSLVec3(float x, float y, float z) { return float3(x, y, z); }
float3 GLSLVec3(float3 x) { return x; }
float3 GLSLVec3(float2 v, float z) { return float3(v.x, v.y, z); }
float3 GLSLVec3(float x, float2 v) { return float3(x, v.x, v.y); }
float3 GLSLVec3(float x) { return float3(x, x, x); }
float4 GLSLVec4(float x, float y, float z, float w) { return float4(x, y, z, w); }
float4 GLSLVec4(float4 x) { return x; }
float4 GLSLVec4(float3 v, float w) { return float4(v.x, v.y, v.z, w); }
float4 GLSLVec4(float x, float3 v) { return float4(x, v.x, v.y, v.z); }
float4 GLSLVec4(float2 v, float2 w) { return float4(v.x, v.y, w.x, w.y); }
float4 GLSLVec4(float x, float y, float2 v) { return float4(x, y, v.x, v.y); }
float4 GLSLVec4(float2 v, float x, float y) { return float4(v.x, v.y, x, y); }
float4 GLSLVec4(float x) { return float4(x, x, x, x); }

int2 GLSLIVec2(int x, int y) { return int2(x, y); }
int2 GLSLIVec2(int2 x) { return x; }
int2 GLSLIVec2(int x) { return int2(x, x); }
int3 GLSLIVec3(int x, int y, int z) { return int3(x, y, z); }
int3 GLSLIVec3(int3 x) { return x; }
int3 GLSLIVec3(int x) { return int3(x, x, x); }
int4 GLSLIVec4(int x, int y, int z, int w) { return int4(x, y, z, w); }
int4 GLSLIVec4(int4 x) { return x; }
int4 GLSLIVec4(int x) { return int4(x, x, x, x); }

// Matrix Helpers
float2x2 GLSLMat2(float s) { float2x2 m; m[0] = float2(s, 0.0); m[1] = float2(0.0, s); return m; }
float2x2 GLSLMat2(float2 c0, float2 c1) { float2x2 m; m[0] = c0; m[1] = c1; return transpose(m); }
float2x2 GLSLMat2(float a, float b, float c, float d) { float2x2 m; m[0] = float2(a, b); m[1] = float2(c, d); return transpose(m); }

float3x3 GLSLMat3(float s) { float3x3 m; m[0] = float3(s, 0.0, 0.0); m[1] = float3(0.0, s, 0.0); m[2] = float3(0.0, 0.0, s); return m; }
float3x3 GLSLMat3(float3 c0, float3 c1, float3 c2) { float3x3 m; m[0] = c0; m[1] = c1; m[2] = c2; return transpose(m); }
float3x3 GLSLMat3(float a, float b, float c, float d, float e, float f, float g, float h, float i)
{
	float3x3 m; m[0] = float3(a, b, c); m[1] = float3(d, e, f); m[2] = float3(g, h, i); return transpose(m);
}

float4x4 GLSLMat4(float s) { float4x4 m; m[0] = float4(s, 0.0, 0.0, 0.0); m[1] = float4(0.0, s, 0.0, 0.0); m[2] = float4(0.0, 0.0, s, 0.0); m[3] = float4(0.0, 0.0, 0.0, s); return m; }
float4x4 GLSLMat4(float4 c0, float4 c1, float4 c2, float4 c3) { float4x4 m; m[0] = c0; m[1] = c1; m[2] = c2; m[3] = c3; return transpose(m); }

float2x2 GLSLInverse(float2x2 m) {
    float det = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    return float2x2(m[1][1], -m[0][1], -m[1][0], m[0][0]) / det;
}
float3x3 GLSLInverse(float3x3 m) {
    float3 r0 = cross(m[1], m[2]);
    float3 r1 = cross(m[2], m[0]);
    float3 r2 = cross(m[0], m[1]);
    float det = dot(m[0], r0);
    return transpose(float3x3(r0, r1, r2) / det);
}
float4x4 GLSLInverse(float4x4 m) {
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];
    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;
    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;
    float4x4 res;
    res[0][0] = t11 * idet; res[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    res[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    res[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;
    res[1][0] = t12 * idet; res[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    res[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    res[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;
    res[2][0] = t13 * idet; res[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    res[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    res[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;
    res[3][0] = t14 * idet; res[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    res[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    res[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;
    return transpose(res);
}

SamplerState sSampler : register(s0);
)";
		}
	};
	void RenderPassShader::bindParameters(ShaderParameterMap const& parameterMap)
	{
		mParamChannelResolution.bind(parameterMap, "iChannelResolution");
		if (passInfo)
		{
			bindInputParameters(passInfo->inputs, parameterMap);
		}
	}

	void RenderPassShader::bindInputParameters(TArray<RenderInput> const& inputs, ShaderParameterMap const& parameterMap)
	{
		mParamInputs.resize(inputs.size());
		mParamInputSamplers.resize(inputs.size());
		int index = 0;
		for (auto const& input : inputs)
		{
			mParamInputs[index].bind(parameterMap, InlineString<>::Make("iChannel%d", input.channel));
			mParamInputSamplers[index].bind(parameterMap, InlineString<>::Make("iChannel%dSampler", input.channel));
			++index;
		}
	}

	RHISamplerState& RenderPassShader::GetSamplerState(RenderInput const& input)
	{
		if (input.type == EInputType::Keyboard)
			return TStaticSamplerState< ESampler::Point, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();

		switch (input.filter)
		{
		default:
			break;
		case ESampler::Point:
			switch (input.addressMode)
			{
			case ESampler::Wrap:
				return TStaticSamplerState< ESampler::Point, ESampler::Wrap, ESampler::Wrap, ESampler::Wrap>::GetRHI();
			case ESampler::Clamp:
				return TStaticSamplerState< ESampler::Point, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
			}
			break;
		case ESampler::Bilinear:
			switch (input.addressMode)
			{
			case ESampler::Wrap:
				return TStaticSamplerState< ESampler::Bilinear, ESampler::Wrap, ESampler::Wrap, ESampler::Wrap>::GetRHI();
			case ESampler::Clamp:
				return TStaticSamplerState< ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
			}
			break;
		case ESampler::Trilinear:
			switch (input.addressMode)
			{
			case ESampler::Wrap:
				return TStaticSamplerState< ESampler::Trilinear, ESampler::Wrap, ESampler::Wrap, ESampler::Wrap>::GetRHI();
			case ESampler::Clamp:
				return TStaticSamplerState< ESampler::Trilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
			}
			break;
		}

		return TStaticSamplerState< ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp >::GetRHI();
	}

	RHITextureBase* RenderPassShader::GetInputTexture(RenderInput const& input, TArray< RHITextureRef > const& inputResources)
	{
		if (inputResources.isValidIndex(input.resId) && inputResources[input.resId].isValid())
		{
			return inputResources[input.resId];
		}

		switch (input.type)
		{
		case EInputType::Buffer:
			return GBlackTexture2D;
		case EInputType::Texture:
			return GBlackTexture2D;
		case EInputType::CubeMap:
			return GBlackTextureCube;
		case EInputType::Volume:
			return GBlackTexture3D;
		case EInputType::Webcam:
		case EInputType::Micrphone:
		case EInputType::Soundcloud:
		case EInputType::Video:
		case EInputType::Music:
			break;
		}

		return nullptr;
	}

	void RenderPassShader::setInputParameters(RHICommandList& commandList, TArray<RenderInput> const& inputs, TArray< RHITextureRef > const& inputResources)
	{
		Vector3 channelSize[4] = { Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0) };
		int index = 0;
		for (auto const& input : inputs)
		{
			RHITextureBase* texture = GetInputTexture(input, inputResources);
			if (mParamInputs[index].isBound() && texture)
			{
				if (mParamInputSamplers[index].isBound())
				{
					setTexture(commandList, mParamInputs[index], *texture, mParamInputSamplers[index], GetSamplerState(input));
				}
				else
				{
					setTexture(commandList, mParamInputs[index], *texture);
				}

				channelSize[input.channel] = Vector3(texture->getDesc().dimension);
			}
			++index;
		}

		if (mParamChannelResolution.isBound())
		{
			setParam(commandList, mParamChannelResolution, channelSize, ARRAY_SIZE(channelSize));
		}
	}

	void RenderPassShader::setSoundParameters(RHICommandList& commandList, float timeOffset, int64 sampleOffset)
	{
		setParam(commandList, SHADER_PARAM(iTimeOffset), timeOffset);
		setParam(commandList, SHADER_PARAM(iSampleOffset), (int32)sampleOffset);
	}

	void RenderPassShader::setCubeParameters(RHICommandList& commandList, IntVector2 const& viewportSize, int subPassIndex /*= INDEX_NONE*/)
	{
		if (subPassIndex == INDEX_NONE)
		{
			Vector3 const* faceDirs = ETexture::GetFaceDirArray();
			Vector3 const* faceUpDirs = ETexture::GetFaceUpArray();

			static Vector3 const faceRightDirs[ETexture::FaceCount] =
			{
				faceDirs[0].cross(faceUpDirs[0]),
				faceDirs[1].cross(faceUpDirs[1]),
				faceDirs[2].cross(faceUpDirs[2]),
				faceDirs[3].cross(faceUpDirs[3]),
				faceDirs[4].cross(faceUpDirs[4]),
				faceDirs[5].cross(faceUpDirs[5]),
			};

			setParam(commandList, SHADER_PARAM(iFaceDirs), faceDirs, ETexture::FaceCount);
			setParam(commandList, SHADER_PARAM(iFaceVDirs), faceUpDirs, ETexture::FaceCount);
			setParam(commandList, SHADER_PARAM(iFaceUDirs), faceRightDirs, ETexture::FaceCount);
		}
		else
		{
			Vector3 faceDir = ETexture::GetFaceDir(ETexture::Face(subPassIndex));
			Vector3 faceUpDir = ETexture::GetFaceUpDir(ETexture::Face(subPassIndex));
			setParam(commandList, SHADER_PARAM(iFaceDir), faceDir);
			setParam(commandList, SHADER_PARAM(iFaceVDir), faceUpDir);
			setParam(commandList, SHADER_PARAM(iFaceUDir), faceDir.cross(faceUpDir));
			setParam(commandList, SHADER_PARAM(iOrigin), Vector3(0, 0, 0));
			setParam(commandList, SHADER_PARAM(iViewportSize), Vector2(viewportSize));
		}

		setParam(commandList, SHADER_PARAM(iOrigin), Vector3(0, 0, 0));
		setParam(commandList, SHADER_PARAM(iViewportSize), Vector2(viewportSize));
	}

	RHITexture3D* LoadBinTexture(char const* path, bool bGenMipmap)
	{
		InputFileSerializer serializer;
		if (!serializer.openNoHeader(path))
		{
			return nullptr;
		}

		struct Header
		{
			char magic[4];
			uint32 sizeX;
			uint32 sizeY;
			uint32 sizeZ;

			uint8  numChannels;
			uint8  layout;
			uint16 format;
		};


		Header header;
		serializer >> header;

		bool bIsFloat = (header.format == 10);
		uint32 srcPixelByte = header.numChannels * (bIsFloat ? sizeof(float) : sizeof(uint8));
		uint32 dstNumChannels = (header.numChannels == 3) ? 4 : header.numChannels;
		uint32 dstPixelByte = dstNumChannels * (bIsFloat ? sizeof(float) : sizeof(uint8));

		uint32 numPixels = header.sizeX * header.sizeY * header.sizeZ;
		TArray<uint8> data;
		data.resize(numPixels * dstPixelByte);

		if (header.numChannels == dstNumChannels)
		{
			serializer.read(data.data(), data.size());
		}
		else
		{
			TArray<uint8> srcData;
			srcData.resize(numPixels * srcPixelByte);
			serializer.read(srcData.data(), srcData.size());

			if (bIsFloat)
			{
				float* pSrc = (float*)srcData.data();
				float* pDst = (float*)data.data();
				for (uint32 i = 0; i < numPixels; ++i)
				{
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst[3] = 1.0f;
					pSrc += 3;
					pDst += 4;
				}
			}
			else
			{
				uint8* pSrc = srcData.data();
				uint8* pDst = data.data();
				for (uint32 i = 0; i < numPixels; ++i)
				{
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst[3] = 255;
					pSrc += 3;
					pDst += 4;
				}
			}
		}

		ETexture::Format format;
		switch (dstNumChannels + (bIsFloat ? 10 : 0))
		{
		case 1:  format = ETexture::R8; break;
		case 2:  format = ETexture::RG8; break;
		case 4:  format = ETexture::RGBA8; break;
		case 11: format = ETexture::R32F; break;
		case 12: format = ETexture::RG32F; break;
		case 14: format = ETexture::RGBA32F; break;
		default:
			return nullptr;
		}

		TextureDesc desc = TextureDesc::Type3D(format, header.sizeX, header.sizeY, header.sizeZ);
		if (bGenMipmap)
		{
			desc.numMipLevel = RHIUtility::CalcMipmapLevel(Math::Min(header.sizeX, Math::Min(header.sizeY, header.sizeZ)));
			desc.AddFlags(TCF_GenerateMips);
		}
		else
		{
			desc.numMipLevel = 1;
		}

		return RHICreateTexture3D(desc, data.data());
	}

	void Renderer::setRenderTarget(RHICommandList& commandList, RHITexture2D& target)
	{
		mFrameBuffer->setTexture(0, target);
		RHISetFrameBuffer(commandList, mFrameBuffer);
	}

	void Renderer::setPixelShader(RHICommandList& commandList, RenderPassShader& shader)
	{
		GraphicsShaderStateDesc state;
		state.vertex = mScreenVS->getRHI();
		state.pixel = shader.getRHI();
		RHISetGraphicsShaderBoundState(commandList, state);
	}

	void Renderer::updateInput(InputParam const& inputParam, uint8 keyboardData[])
	{
		mInputBuffer.updateBuffer(inputParam);
		RHIUpdateTexture(*mTexKeyboard, 0, 0, 256, 3, keyboardData);
	}

	void Renderer::setInputParameters(RHICommandList& commandList, RenderPassData& pass)
	{
		pass.shader.setInputParameters(commandList, pass.info->inputs, mInputResources);
		SetStructuredUniformBuffer(commandList, pass.shader, mInputBuffer);
	}

	PooledRenderTargetRef Renderer::render(RHICommandList& commandList, TArray< std::unique_ptr<RenderPassData> > const& renderPassList, IntVector2 const& screenSize)
	{
		GPU_PROFILE(bAllowComputeShader ? "RenderPassCS" : "RenderPass");

		PooledRenderTargetRef outImage;
		int const CubeMapSize = 1024;

		for (auto& pass : renderPassList)
		{
			if (pass->info->passType == EPassType::Sound)
			{
				continue;
			}

			bool bUseComputeShader = pass->shader.getType() == EShader::Compute;
			bool bRenderCubeMap = pass->info->passType == EPassType::CubeMap;

			bool bRenderCubeMapOnePass = bRenderCubeMap && bAllowRenderCubeOnePass;

			TArray< PooledRenderTargetRef, TFixedAllocator<4> > outputBuffers;
			HashString debugName;
			int index;
			IntVector2 viewportSize = bRenderCubeMap ? IntVector2(CubeMapSize, CubeMapSize) : screenSize;

			RenderTargetDesc desc;
			desc.format = ETexture::RGBA32F;
			desc.numSamples = 1;
			if (bUseMultisample && pass->info->passType == EPassType::Image)
			{
				desc.numSamples = 8;
			}
			desc.size = viewportSize;
			desc.type = bRenderCubeMap ? ETexture::TypeCube : ETexture::Type2D;

			if (bUseComputeShader)
			{
				desc.creationFlags |= TCF_CreateUAV;
			}

			index = 0;
			for (auto const& output : pass->info->outputs)
			{
				if (index == 0)
				{
					switch (pass->info->passType)
					{
					case EPassType::Buffer:
						{
							desc.debugName = InlineString<>::Make("Buffer%c", AlphaSeq[pass->info->typeIndex]);
						}
						break;
					case EPassType::CubeMap:
						{
							desc.debugName = InlineString<>::Make("CubeMap%c", AlphaSeq[pass->info->typeIndex]);
						}
						break;
					case EPassType::Image:
						{
							desc.debugName = "Image";
						}
						break;
					}

					if (bRenderCubeMapOnePass)
					{
						debugName = InlineString<>::Make("CubeMap%c OnePass", AlphaSeq[pass->info->typeIndex]);
					}
					else
					{
						debugName = desc.debugName;
					}
				}
				else
				{

					switch (pass->info->passType)
					{
					case EPassType::Buffer:
						{
							desc.debugName = InlineString<>::Make("Buffer%c(%d)", AlphaSeq[pass->info->typeIndex], output.channel);
						}
						break;
					case EPassType::CubeMap:
						{
							desc.debugName = InlineString<>::Make("CubeMap%c(%d)", AlphaSeq[pass->info->typeIndex], output.channel);
						}
						break;
					case EPassType::Image:
						{
							desc.debugName = InlineString<>::Make("Image(%d)", output.channel);
						}
						break;
					}
				}

				PooledRenderTargetRef rt;
				rt = GRenderTargetPool.fetchElement(desc);
				outputBuffers.push_back(rt);
				++index;
			}

			GPU_PROFILE(debugName.c_str());

			if (bRenderCubeMapOnePass)
			{
				renderCubeOnePass(commandList, *pass, viewportSize, outputBuffers);
			}
			else
			{
				renderImage(commandList, *pass, viewportSize, outputBuffers);
			}
			if (pass->info->passType == EPassType::Image)
			{
				outImage = outputBuffers[0];
				if (outImage->desc.numSamples > 1)
				{
					outImage->resolve(commandList);
				}
			}
			else
			{
				index = 0;
				for (auto const& output : pass->info->outputs)
				{
					auto& buffer = mOutputBuffers[output.bufferId];
					if (buffer.isValid())
					{
						buffer->desc.debugName = EName::None;
						buffer->bResvered = false;
					}

					buffer = outputBuffers[index];
					buffer->bResvered = true;

					if (buffer->desc.numSamples > 1)
					{
						buffer->resolve(commandList);
					}
					++index;

					mInputResources[output.resId] = buffer->resolvedTexture;
				}
			}
		}

		return outImage;
	}

	void Renderer::renderImage(RHICommandList& commandList, RenderPassData& pass, IntVector2 const& viewportSize, TArrayView< PooledRenderTargetRef > outputBuffers)
	{
		int index;
		auto& passShader = pass.shader;

		auto SetPassParameters = [&](RHICommandList& commandList, int subPassIndex)
		{
			if (pass.info->passType == EPassType::CubeMap)
			{
				passShader.setCubeParameters(commandList, viewportSize, subPassIndex);
			}
		};

		int subPassCount = pass.info->passType == EPassType::CubeMap ? 6 : 1;

		if (passShader.getType() == EShader::Compute)
		{
			RHISetComputeShader(commandList, pass.shader.getRHI());
			setInputParameters(commandList, pass);

			for (int subPassIndex = 0; subPassIndex < subPassCount; ++subPassIndex)
			{
				SetPassParameters(commandList, subPassIndex);

				if (subPassCount == 1)
				{
					index = 0;
					for (auto const& output : pass.info->outputs)
					{
						passShader.setRWTexture(commandList, GetOutputName(output.channel), *outputBuffers[index]->resolvedTexture, 0, EAccessOp::WriteOnly);
						++index;
					}
				}
				else
				{
					index = 0;
					for (auto const& output : pass.info->outputs)
					{
						passShader.setRWSubTexture(commandList, GetOutputName(output.channel), *outputBuffers[index]->resolvedTexture, subPassIndex, 0, EAccessOp::WriteOnly);
						++index;
					}
				}

				for (int i = 0; i < pass.info->outputs.size(); ++i)
				{
					RHIResourceTransition(commandList, { outputBuffers[i]->resolvedTexture }, EResourceTransition::UAV);
				}
				RHIDispatchCompute(commandList, Math::AlignCount(viewportSize.x, GROUP_SIZE), Math::AlignCount(viewportSize.y, GROUP_SIZE), 1);
				for (int i = 0; i < pass.info->outputs.size(); ++i)
				{
					RHIResourceTransition(commandList, { outputBuffers[i]->resolvedTexture }, EResourceTransition::SRV);
				}
			}

			index = 0;
			for (auto const& output : pass.info->outputs)
			{
				passShader.clearRWTexture(commandList, GetOutputName(output.channel));
				++index;
			}
		}
		else
		{
			RHISetViewport(commandList, 0, 0, viewportSize.x, viewportSize.y);

			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

			setPixelShader(commandList, passShader);
			setInputParameters(commandList, pass);

			for (int subPassIndex = 0; subPassIndex < subPassCount; ++subPassIndex)
			{
				index = 0;
				for (auto const& output : pass.info->outputs)
				{
					mFrameBuffer->setTexture(output.channel, *outputBuffers[index]->texture, subPassIndex);
					++index;
				}

				RHISetFrameBuffer(commandList, mFrameBuffer);
				SetPassParameters(commandList, subPassIndex);
				DrawUtility::ScreenRect(commandList);
				RHISetFrameBuffer(commandList, nullptr);
			}
		}
	}

	void Renderer::renderCubeOnePass(RHICommandList& commandList, RenderPassData& pass, IntVector2 const& viewportSize, TArrayView< PooledRenderTargetRef > outputBuffers)
	{
		int index;
		auto& passShader = pass.shader;

		auto SetCubeParameters = [&](RHICommandList& commandList, IntVector2 const& viewportSize)
		{
			Vector3 const* faceDirs = ETexture::GetFaceDirArray();
			Vector3 const* faceUpDirs = ETexture::GetFaceUpArray();

			static Vector3 const faceRightDirs[ETexture::FaceCount] =
			{
				faceDirs[0].cross(faceUpDirs[0]),
				faceDirs[1].cross(faceUpDirs[1]),
				faceDirs[2].cross(faceUpDirs[2]),
				faceDirs[3].cross(faceUpDirs[3]),
				faceDirs[4].cross(faceUpDirs[4]),
				faceDirs[5].cross(faceUpDirs[5]),
			};

			passShader.setParam(commandList, SHADER_PARAM(iFaceDirs), faceDirs, ETexture::FaceCount);
			passShader.setParam(commandList, SHADER_PARAM(iFaceVDirs), faceUpDirs, ETexture::FaceCount);
			passShader.setParam(commandList, SHADER_PARAM(iFaceUDirs), faceRightDirs, ETexture::FaceCount);

			passShader.setParam(commandList, SHADER_PARAM(iOrigin), Vector3(0, 0, 0));
			passShader.setParam(commandList, SHADER_PARAM(iViewportSize), Vector2(viewportSize));
		};

		if (passShader.getType() == EShader::Compute)
		{
			RHISetComputeShader(commandList, pass.shader.getRHI());
			setInputParameters(commandList, pass);
			passShader.setCubeParameters(commandList, viewportSize);

			index = 0;
			for (auto const& output : pass.info->outputs)
			{
				passShader.setRWTexture(commandList, GetOutputName(output.channel), *outputBuffers[index]->resolvedTexture, 0, EAccessOp::WriteOnly);
				++index;
			}

			for (int i = 0; i < pass.info->outputs.size(); ++i)
			{
				RHIResourceTransition(commandList, { outputBuffers[i]->resolvedTexture }, EResourceTransition::UAV);
			}

			RHIDispatchCompute(commandList, Math::AlignCount(viewportSize.x, GROUP_SIZE), Math::AlignCount(viewportSize.y, GROUP_SIZE), 6);
			for (int i = 0; i < pass.info->outputs.size(); ++i)
			{
				RHIResourceTransition(commandList, { outputBuffers[i]->resolvedTexture }, EResourceTransition::SRV);
			}

			index = 0;
			for (auto const& output : pass.info->outputs)
			{
				passShader.clearRWTexture(commandList, GetOutputName(output.channel));
				++index;
			}
		}
		else
		{
			RHISetViewport(commandList, 0, 0, viewportSize.x, viewportSize.y);

			index = 0;
			for (auto const& output : pass.info->outputs)
			{
				mFrameBuffer->setTextureArray(output.channel, static_cast<RHITextureCube&>(*outputBuffers[index]->texture));
				++index;
			}

			RHISetFrameBuffer(commandList, mFrameBuffer);

			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

			GraphicsShaderStateDesc state;
			state.vertex = mCubeOnePassVS->getRHI();
			state.pixel = passShader.getRHI();
			RHISetGraphicsShaderBoundState(commandList, state);

			setInputParameters(commandList, pass);
			passShader.setCubeParameters(commandList, viewportSize);

			DrawUtility::ScreenRect(commandList, ETexture::FaceCount);
			RHISetFrameBuffer(commandList, nullptr);
		}
	}

	void Renderer::renderSound(RHICommandList& commandList, RenderPassData& pass, WaveFormatInfo const& format, int64 samplePos, int sampleCount, AudioStreamSample& outData)
	{
		auto& passShader = pass.shader;

		int blockFrame = SoundTextureSize * SoundTextureSize;

		int numBlocks = Math::AlignCount(sampleCount, blockFrame);

		if (passShader.getType() == EShader::Compute)
		{
			RHISetComputeShader(commandList, pass.shader.getRHI());
			setInputParameters(commandList, pass);

			passShader.setRWTexture(commandList, GetOutputName(0), *mTexSound, 0, EAccessOp::WriteOnly);

			int16* pData = (int16*)outData.data;
			for (int indexBlock = 0; indexBlock < numBlocks; ++indexBlock)
			{
				int64 sampleOffset = samplePos + indexBlock * blockFrame;
				float timeOffset = float(sampleOffset) / format.sampleRate;
				passShader.setSoundParameters(commandList, timeOffset, sampleOffset);

				int dispatchCount = Math::AlignCount(SoundTextureSize, GROUP_SIZE);
				RHIResourceTransition(commandList, { mTexSound }, EResourceTransition::UAV);
				RHIDispatchCompute(commandList, dispatchCount, dispatchCount, 1);
				RHIResourceTransition(commandList, { mTexSound }, EResourceTransition::SRV);
				RHIFlushCommand(commandList);

				int numRead = Math::Min(sampleCount - indexBlock * blockFrame, blockFrame);
				readSampleData(pData, numRead);
				pData += numRead * format.numChannels;
			}

			passShader.clearRWTexture(commandList, GetOutputName(0));
		}
		else
		{
			setRenderTarget(commandList, *mTexSound);

			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 0), 1);
			RHISetViewport(commandList, 0, 0, SoundTextureSize, SoundTextureSize);

			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

			setPixelShader(commandList, passShader);
			setInputParameters(commandList, pass);

			int16* pData = (int16*)outData.data;
			for (int indexBlock = 0; indexBlock < numBlocks; ++indexBlock)
			{
				int64 sampleOffset = samplePos + indexBlock * blockFrame;
				float timeOffset = float(sampleOffset) / format.sampleRate;
				passShader.setSoundParameters(commandList, timeOffset, sampleOffset);

				DrawUtility::ScreenRect(commandList);
				RHIFlushCommand(commandList);

				int numRead = Math::Min(sampleCount - indexBlock * blockFrame, blockFrame);
				readSampleData(pData, numRead);
				pData += numRead * format.numChannels;
			}
		}
	}

	bool Renderer::initializeRHI()
	{
		mFrameBuffer = RHICreateFrameBuffer();

		ScreenVS::PermutationDomain domainVector;
		domainVector.set<ScreenVS::UseTexCoord>(false);
		mScreenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(domainVector);
		mCubeOnePassVS = ShaderManager::Get().getGlobalShaderT<CubeOnePassVS>();

		mTexSound = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, SoundTextureSize, SoundTextureSize).AddFlags(TCF_AllowCPUAccess | TCF_CreateUAV | TCF_RenderTarget));
		GTextureShowManager.registerTexture("SoundTexture", mTexSound);
		mTexKeyboard = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R8, 256, 3));
		GTextureShowManager.registerTexture("Keyborad", mTexKeyboard);
		mInputBuffer.initializeResource(1);
		return true;
	}

	void Renderer::releaseRHI()
	{
		mFrameBuffer.release();
		mScreenVS = nullptr;
		mTexSound.release();
		mTexKeyboard.release();
		mInputBuffer.releaseResource();
	}

	void Renderer::readSampleData(int16* pOutData, int numSamples)
	{
		TArray<uint8> data;
		RHIReadTexture(*mTexSound, ETexture::RGBA8, 0, data);

		uint8 const* pData = data.data();
		for (int i = 0; i < numSamples; ++i)
		{
			auto sample = Decode(pData);
			pData += 4;

			pOutData[0] = sample.x;
			pOutData[1] = sample.y;
			pOutData += 2;
		}
	}

	void Renderer::addInputResource(RenderInput const& input, RHITextureBase& texture)
	{
		if (mInputResources.size() < input.resId + 1)
		{
			mInputResources.resize(input.resId + 1);
		}
		mInputResources[input.resId] = &texture;
		GTextureShowManager.registerTexture(InlineString<>::Make("Res%d", input.resId).c_str(), &texture);
	}

	bool Renderer::compileShader(RenderPassData& pass, TArrayView<uint8 const> codeTemplate, RenderPassInfo* commonInfo)
	{
		std::string channelCode;
		for (RenderInput const& input : pass.info->inputs)
		{
			switch (input.type)
			{
			case EInputType::Keyboard:
				channelCode += InlineString<>::Make("uniform sampler2D iChannel%d;\n", input.channel);
				break;
			case EInputType::Webcam:
			case EInputType::Micrphone:
			case EInputType::Soundcloud:
			case EInputType::Buffer:
			case EInputType::Texture:
			case EInputType::Video:
			case EInputType::Music:
				channelCode += InlineString<>::Make("uniform sampler2D iChannel%d;\n", input.channel);
				break;
			case EInputType::CubeMap:
				channelCode += InlineString<>::Make("uniform samplerCube iChannel%d;\n", input.channel);
				break;
			case EInputType::Volume:
				channelCode += InlineString<>::Make("uniform sampler3D iChannel%d;\n", input.channel);
				break;
			}
		}

		bool bIsGLSL = true;
		if (GRHISystem->getName() == RHISystemName::D3D11 || GRHISystem->getName() == RHISystemName::D3D12)
		{
			bIsGLSL = false;
		}

		ShaderCompileOption option;
		if (!bIsGLSL)
		{
			option.addDefine(SHADER_PARAM(COMPILER_HLSL), 1);
		}
		else
		{
			option.addDefine(SHADER_PARAM(COMPILER_GLSL), 1);
		}

		bool bUseComputeShader = canUseComputeShader(pass);

		std::string code;

		std::string commonCodeStr = commonInfo ? commonInfo->code : "";
		std::string passCodeStr = pass.info->code;

		// Add #line directive to point back to the original source logic
		if (!bIsGLSL)
		{
			// Reset line number for user code blocks to mitigate offset from helpers
			if (!commonCodeStr.empty())
			{
				commonCodeStr = "#line 1 \"Common\"\n" + commonCodeStr;
			}
			passCodeStr = "#line 1 \"Image\"\n" + passCodeStr;
		}

		Text::Format((char const*)codeTemplate.data(), { StringView(channelCode), StringView(commonCodeStr), StringView(passCodeStr) }, code);

		if (!bIsGLSL)
		{
			// Transform GLSL to HLSL using the dedicated parser
			code = GLSLToHLSLConverter::Run(code);

			// Prepend Helper Functions AFTER conversion to avoid recursive 'fixing' of the helpers
			code = GLSLToHLSLConverter::GetHelperCode() + "\n" + code;
			
			LogMsg("Converted HLSL Shader:\n%s", code.c_str());
		}
		
		option.addCode(code.c_str());

		if (bUseComputeShader)
		{
			if (pass.info->code.find("discard") != std::string::npos ||
				(commonInfo && commonInfo->code.find("discard") != std::string::npos))
			{
				option.addDefine(SHADER_PARAM(COMPUTE_DISCARD_SIM), 1);
			}
			option.addDefine(SHADER_PARAM(GROUP_SIZE), GROUP_SIZE);
		}

		switch (pass.info->passType)
		{
		case EPassType::Buffer:
			option.addDefine(SHADER_PARAM(RENDER_BUFFER), 1);
			break;
		case EPassType::Sound:
			option.addDefine(SHADER_PARAM(RENDER_SOUND), 1);
			option.addDefine(SHADER_PARAM(SOUND_TEXTURE_SIZE), SoundTextureSize);
			break;
		case EPassType::CubeMap:
			option.addDefine(SHADER_PARAM(RENDER_CUBE), 1);
			if (bAllowRenderCubeOnePass)
			{
				option.addDefine(SHADER_PARAM(RENDER_CUBE_ONE_PASS), 1);
			}
			break;
		case EPassType::Image:
			option.addDefine(SHADER_PARAM(RENDER_IMAGE), 1);
			break;
		}

		ShaderEntryInfo entry = bUseComputeShader ? ShaderEntryInfo{ EShader::Compute, "MainCS" } : ShaderEntryInfo{ EShader::Pixel, "MainPS" };

		pass.shader.passInfo = pass.info;
		if (!ShaderManager::Get().loadFile(pass.shader, nullptr, entry, option))
			return false;

		return true;
	}

}//namespace Shadertoy
