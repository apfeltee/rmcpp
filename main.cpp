/*
*
* Usage:
*   $0 < [inputfile] > [outputfile]
*   or
*   $0 [inputfile] [outputfile]
*
*/

#include <filesystem>
#include "rmcpp.h"
#include "../optionparser/optionparser.hpp"

class Preprocessor
{
    public:
        struct MacroDef
        {
            std::string name;
            std::string value;
            bool hasvalue = false;

            MacroDef()
            {
            }

            MacroDef(const std::string& nm): name(nm), value(), hasvalue(false)
            {
            }

            template<typename... ArgsT>
            MacroDef(const std::string& nm, ArgsT&&... args)
            {
                std::stringstream valstrm;
                ((valstrm << args), ...);
                value = valstrm.str();
                if(!value.empty())
                {
                    hasvalue = true;
                }
            }
            
        };

        using MacroTable = std::map<std::string, MacroDef>;

    private:
        std::istream* m_infp;
        std::stringstream m_datastrm;
        /*
        * lookup table containing the first character of all macros defined
        * greatly reduces time searching
        */
        std::vector<int> m_chmap;
        MacroTable m_macrotbl;


    private:
        bool remove_comments()
        {
            CommentStripper cs(CommentStripper::Options(), m_infp);
            return cs.run(m_datastrm);
        }

        bool mightbemacro(int ch)
        {
            for(auto it=m_macrotbl.begin(); it!=m_macrotbl.end(); it++)
            {
                if(it->first[0] == ch)
                {
                    return true;
                }
            }
            return false;
        }

        void make_mmchars()
        {
            for(auto it=m_macrotbl.begin(); it!=m_macrotbl.end(); it++)
            {
                if(std::find(m_chmap.begin(), m_chmap.end(), it->first[0]) == m_chmap.end())
                {
                    m_chmap.push_back(it->first[0]);
                }
            }
        }

        bool process_macros(std::ostream& outfp)
        {
            int currch;
            make_mmchars();
            while(true)
            {
                currch = m_datastrm.get();
                if(currch == EOF)
                {
                    break;
                }
                if(std::find(m_chmap.begin(), m_chmap.end(), currch) != m_chmap.end())
                {
                    
                }
            }
            return true;
        }

    public:
        Preprocessor(std::istream* infp): m_infp(infp)
        {
        }

        void define(const std::string& name)
        {
            m_macrotbl[name] = MacroDef(name);
        }

        template<typename... ArgsT>
        void define(const std::string& name, ArgsT&&... args)
        {
            m_macrotbl[name] = MacroDef(name, args...);
        }

        bool run(std::ostream& outfp)
        {
            int rc;
            rc = 0;
            rc += !remove_comments();
            #if 0
            /* not yet implemented */
            rc += !read_tokens();
            #endif
            rc += !process_macros(outfp);
            return (rc == 0);
        }

};

/*
    MacroProcessor mc(&std::cin);
    mc.define("__MACROPROC__", 1);
    mc.define("assert", {"x"}, "if(!(x)){ fprintf(stderr, \"ASSERTION %s\n\", #x); abort(); } ");
*/

int main(int argc, char** argv)
{
    bool rc;
    bool have_infile;
    bool have_outfile;
    std::string outfilename;
    std::istream* infp;
    std::ostream* outfp;
    CommentStripper::Options opts;
    // default input is standard input, default output is standard output
    infp = &std::cin;
    outfp = &std::cout;
    // track user-supplied file arguments
    have_infile = false;
    have_outfile = false;
    OptionParser prs;
    prs.onUnknownOption([&](const std::string& v)
    {
        std::cerr << "unknown option '" << v << "'!" << std::endl;
        return false;
    });
    prs.on({"-d", "--debug"}, "show debug messages written to standard error", [&]
    {
        opts.use_debugmessages = true;
    });
    prs.on({"-w", "--nowarnings"}, "disable warnings written to standard error", [&]
    {
        opts.use_warningmessages = false;
    });
    prs.on({"-a", "--keepansi"}, "keep ansi C comments (default: remove)", [&]
    {
        opts.allow_ansicomments = false;
    });
    prs.on({"-c", "--keepcpp"}, "keep C++ comments (default: remove)", [&]
    {
        opts.allow_cppcomments = false;
    });
    prs.on({"-p", "--pascal"}, "remove pascal style comments (default: keep)", [&]
    {
        opts.allow_pascalcomments = true;
    });
    prs.on({"-l", "--hash"}, "remove #-style comments (clashes with preprocessor! dangerous)", [&]
    {
        opts.allow_hashcomments = true;
    });
    /* implement me! */
    #if 0
    prs.on({"-x?", "--preprocessor=?"}, "remove comma-separated C-preprocessor tokens (i.e., '-xinclude,import')",
    [&](const OptionParser::Value& val)
    {
        // -x can be specified several times!
        Util::split<char>(val.str(), ",", [&](const std::string& tok)
        {
            opts.ppctokens.push_back(tok);
        });
        if(opts.ppctokens.size() > 0)
        {
            opts.have_ppctokens = true;
        }
    });
    #endif
    try
    {
        prs.parse(argc, argv);
        auto pos = prs.positional();
        /*
        * merely assigning to a pointer ref would break RTTI:
        * the stream would go out of scope, and the file would be closed.
        * so despite all, the streams need to be new'd. :-b
        */
        if(pos.size() > 0)
        {
            opts.infilename = pos[0];
            infp = new std::ifstream(opts.infilename, std::ios::in | std::ios::binary);
            if(!infp->good())
            {
                Util::error("cannot open %q for reading", opts.infilename);
                return 1;
            }
            have_infile = true;
        }
        if(pos.size() > 1)
        {
            outfilename = pos[1];
            if(std::filesystem::exists(outfilename))
            {
                /* ensure we do not accidently clobber the input file! */
                if(std::filesystem::equivalent(opts.infilename, outfilename))
                {
                    Util::error("outputfile %q is also inputfile!", outfilename);
                    return 1;
                }
            }
            outfp = new std::ofstream(outfilename, std::ios::out | std::ios::binary);
            if(!outfp->good())
            {
                Util::error("cannot open '%s' for writing", outfilename);
                return 1;
            }
            have_outfile = true;
        }

    }
    catch(std::runtime_error& ex)
    {
        std::cerr << "error: " << ex.what() << std::endl;
    }
    CommentStripper x(opts, infp);
    rc = x.run(*outfp);
    if(have_infile)
    {
        delete infp;
    }
    if(have_outfile)
    {
        delete outfp;
    }
    return !rc;
}
