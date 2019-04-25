
#ifndef GLSLUTILS_HPP
#define GLSLUTILS_HPP

#include <boost/filesystem/path.hpp>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include "GLmm/Program.hpp"
#include "Common.hpp"

void LoadComboShaderSource( const boost::filesystem::path& Filename,
						   std::vector<boost::tuple<uint,std::string>>& Sections );

/** Compiles the given source file and attaches them to the program object.
	Does not link!
*/
void CompileShader( const boost::filesystem::path& Filename, GLmm::Program& Result );

/** Compiles the given source file and attaches them to the program object.
	Adds the given defines to each source file.
	Does not link!
*/
void CompileShader( const boost::filesystem::path& Filename,
	const std::vector<std::string>& Defines, GLmm::Program& Result );

#endif
