#include "com_misc.h"

#include <filesystem>
#include <fstream>
#include <list>
#include <sstream>
#include <string>

#include "com_misc.h"
#include "universal/core_assert.h"
#include "universal/dvar_api.h"
#include "universal/print.h"

#define DEFINE_DVAR
#include "com_dvars.h"

void Com_RegisterDvars()
{
#define REGISTER_DVAR
#include "com_dvars.h"
}

using std::list;
using std::string;

class CommandHelper {
    list<string> commands_;

   public:
    void SetFromCommandLine( int argc, const char** argv )
    {
        for ( int idx = 0; idx < argc; ++idx )
        {
            commands_.emplace_back( string( argv[idx] ) );
        }
    }

    void PushCfg( const char* file )
    {
        core_assert( 0 && "TODO: implement" );
        char path[256];
        // Com_FsBuildPath( path, sizeof( path ), file, "scripts" );

        if ( !std::filesystem::exists( path ) )
        {
            Com_PrintWarning( "[filesystem] file '%s' does not exist", path );
            return;
        }

        std::ifstream fs( path );
        list<string> cfg;
        string line;
        while ( std::getline( fs, line ) )
        {
            std::istringstream iss( line );
            string token;
            if ( iss >> token )
            {
                if ( token.front() == '#' )
                {
                    continue;
                }
            }

            do
            {
                cfg.emplace_back( token );
            } while ( iss >> token );
        }
        cfg.insert( cfg.end(), commands_.begin(), commands_.end() );
        commands_ = std::move( cfg );
    }

    bool TryConsume( string& str )
    {
        if ( commands_.empty() )
        {
            str.clear();
            return false;
        }

        str = commands_.front();
        commands_.pop_front();
        return true;
    }

    bool Consume( string& str )
    {
        if ( commands_.empty() )
        {
            Com_PrintError( "Unexpected EOF" );
            str.clear();
            return false;
        }

        return TryConsume( str );
    }
};

bool Com_ProcessCmdLine( int argc, const char** argv )
{
    CommandHelper cmdHelper;
    cmdHelper.SetFromCommandLine( argc, argv );

    string str;
    while ( cmdHelper.TryConsume( str ) )
    {
        if ( str == "+set" )
        {
            cmdHelper.Consume( str );
            dvar_t* dvar = Dvar_FindByName_Internal( str.c_str() );
            if ( dvar == nullptr )
            {
                Com_PrintError( "[dvar] Dvar '%s' not found", str.c_str() );
                return false;
            }
            cmdHelper.Consume( str );
            Dvar_SetFromString_Internal( *dvar, str.c_str() );
        }
        else if ( str == "+exec" )
        {
            cmdHelper.Consume( str );
            Com_PrintInfo( "Executing '%s'", str.c_str() );
            cmdHelper.PushCfg( str.c_str() );
        }
        else
        {
            Com_PrintError( "Unknown command '%s'", str.c_str() );
            return false;
        }
    }

    Com_PrintSuccess( "cmd line processed" );
    return true;
}