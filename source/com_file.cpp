#include "com_file.h"

#include <fstream>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>

#include "universal/print.h"

using std::any_cast;
using std::getline;
using std::ifstream;
using std::istreambuf_iterator;
using std::istringstream;
using std::runtime_error;
using std::stack;
using std::string;
using std::to_string;

static string BasePath( const char* path )
{
    const char* end = strrchr( path, '/' );
    return string( path, end ? end - path + 1 : 0 );
}

string ReadAsciiFile( const char* path )
{
    ifstream ifs( path );
    if ( !ifs.is_open() )
    {
        Com_PrintError( "[filesystem] failed to open '%s'", path );
        return "";
    }

    return string( istreambuf_iterator<char>( ifs ),
                   istreambuf_iterator<char>() );
}

string ReadAsciiFile( const string& path )
{
    return ReadAsciiFile( path.c_str() );
}

string PreprocessFile( const char* path, const DefineList& defines )
{
    string buffer;

    for ( const auto& define : defines )
    {
        buffer.append( "#define " ).append( define.first ).push_back( ' ' );
        const type_info& type = define.second.type();
        if ( type == typeid( int ) )
        {
            buffer.append( to_string( any_cast<int>( define.second ) ) );
        }
        else if ( type == typeid( size_t ) )
        {
            buffer.append( to_string( any_cast<size_t>( define.second ) ) );
        }
        else
        {
            Com_PrintWarning( "[filesystem] unrecognized #define %s %s", define.first.c_str(), type.name() );
        }
        buffer.push_back( '\n' );
    }

    string line;
    stack<istringstream> streams;
    streams.push( istringstream( ReadAsciiFile( path ) ) );
    while ( !streams.empty() )
    {
        istringstream& stream = streams.top();
        bool interrupt        = false;
        while ( getline( stream, line ) )
        {
            constexpr char pattern[] = "#include \"";
            const char* lineStart    = line.c_str();
            if ( strncmp( lineStart, pattern, sizeof( pattern ) - 1 ) == 0 )
            {
                // open new file
                const char* begin = lineStart + sizeof( pattern ) - 1;
                const char* end   = strchr( begin, '"' );
                string base       = BasePath( path );
                string file( begin, end ? end - begin : 0 );
                streams.push( istringstream( ReadAsciiFile( base + file ) ) );
                interrupt = true;
                break;
            }

            buffer.append( line ).push_back( '\n' );
        }

        if ( !interrupt )
        {
            streams.pop();
        }
    }

    return buffer;
}

string PreprocessFile( const string& path, const DefineList& defines )
{
    return PreprocessFile( path.c_str(), defines );
}
