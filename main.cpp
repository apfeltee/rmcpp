/**
* based on stripcomments.c, written by Stephan Beal (stephan@wanderinghorse.net),
* which is in the public domain. Thanks mate.
*
* NB. things like (ignoring (pad))
*     /\(pad)
*(pad)/ c++ comment
* (a dumb way to write C++ comments)
*
* or
*
* /\(pad)
*(pad)*
* (a dumb way to write ANSI C comments)
*
* will NOT be implemented. not just because it makes the processing
* logic unnecessarily more complex, but also because apart from GCC,
* most preprocessors do not implement it either, or warn against it.
* and why would they? it's hare-brained, to put it mildly.
*
*
* A quick hack to strip C- and C++-style comments from stdin, sending
* the results to stdout. It assumes that its input is legal C-like
* code, and does only little error handling.
*
* It treats strings as anything starting and ending with matching
* double OR single quotes (for use with scripting languages which use
* both). It should not be used on any code which might contain C/C++
* comments inside heredocs, and similar constructs, as it will strip
* those out.
*
* Usage: $0 < input > output
*
*/

#include <functional>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include "../optionparser/optionparser.hpp"

namespace Util
{
    // based on

    // trim from start (in place)
    template<typename CharT>
    void ltrim(std::basic_string<CharT>& str)
    {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch)
        {
            return !std::isspace(ch);
        }));
    }

    // trim from end (in place)
    template<typename CharT>
    void rtrim(std::basic_string<CharT>& str)
    {
        str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch)
        {
            return !std::isspace(ch);
        }).base(), str.end());
    }

    // trim from both ends (in place)
    template<typename CharT>
    void trim(std::basic_string<CharT>& str)
    {
        ltrim(str);
        rtrim(str);
    }

    void split(const std::string& str, const std::string& delim, std::function<void(const std::string&)> cb)
    {
        size_t prevpos;
        size_t herepos;
        std::string token;
        prevpos = 0;
        herepos = 0;
        while(true)
        {
            herepos = str.find(delim, prevpos);
            if(herepos == std::string::npos)
            {
                herepos = str.size();
            }
            token = str.substr(prevpos, herepos-prevpos);
            trim(token);
            if (!token.empty())
            {
                cb(token);
            }
            prevpos = herepos + delim.size();
            if((herepos > str.size()) || (prevpos > str.size()))
            {
                break;
            }
        }
    }

    template<typename CharT>
    void escapechar(std::basic_ostream<CharT>& out, int ch, bool wquotes=false)
    {
        if((ch >= 32) && (ch <= 126))
        {
            if(ch == '\\')
            {
                out << CharT('\\') << CharT('\\');
            }
            else
            {
                out << CharT(ch);
            }
            return;
        }
        else
        {
            if((ch == CharT('"')) && wquotes)
            {
                out << CharT('\\') << CharT('"');
                return;
            }
            switch(ch)
            {
                case 0:    out << CharT('\\') << CharT('0'); return;
                case 1:    out << CharT('\\') << CharT('1'); return;
                case '\n': out << CharT('\\') << CharT('n'); return;
                case '\r': out << CharT('\\') << CharT('r'); return;
                case '\t': out << CharT('\\') << CharT('t'); return;
                case '\f': out << CharT('\\') << CharT('f'); return;
            }
            out
                << CharT('\\')
                << CharT('x')
                << std::setfill('0')
                << std::setw(2)
                << std::uppercase
                << std::hex
                << CharT(ch)
                << std::resetiosflags(std::ios_base::basefield)
            ;
        }
    }

    template<typename CharT>
    void escapestring(std::basic_ostream<CharT>& out, const std::basic_string<CharT>& str, bool wquotes=false)
    {
        if(wquotes)
        {
            out << CharT('"');
        }
        for(CharT ch: str)
        {
            escapechar(out, ch, wquotes);
        }
        if(wquotes)
        {
            out << CharT('"');
        }
    }

    template<typename CharT>
    std::basic_ostream<CharT>&
    sfprintf(
        std::basic_ostream<CharT>& out,
        const std::string& format)
    {
        out << format;
        return out;
    }
     
    template<typename CharT, typename Type, typename... Targs>
    std::basic_ostream<CharT>& sfprintf(
        std::basic_ostream<CharT>& out,
        const std::string& fmt,
        Type value,
        Targs... Fargs
    )
    {
        int cd;
        size_t i;
        for (i=0; i<fmt.size(); i++)
        {
            if(fmt[i] == '%')
            {
                cd = fmt[i + 1];
                switch(cd)
                {
                    case '%':
                        out << CharT('%');
                        break;
                    case 's':
                    case 'd':
                    case 'l':
                    case 'c':
                        out << value;
                        break;
                    case 'p':
                        {
                            std::basic_stringstream<CharT> tmp;
                            tmp << value;
                            escapestring(out, tmp.str(), false);
                        }
                        break;
                    case 'q':
                        {
                            std::basic_stringstream<CharT> tmp;
                            tmp << value;
                            escapestring(out, tmp.str(), true);
                        }
                        break;
                    default:
                        {
                            std::stringstream b;
                            b << "invalid format flag '" << char(cd) << "'";
                            throw std::runtime_error(b.str());
                        }
                        break;
                }
                auto subst = fmt.substr(i+2, fmt.length());
                return sfprintf(out, subst, Fargs...);
            }
            out << fmt[i];
        }
        return out;
    }

    template<typename... Args>
    void error(const std::string& fmtstr, Args&&... args)
    {
        sfprintf(std::cerr, "ERROR: ");
        sfprintf(std::cerr, fmtstr, args...);
        std::cerr << std::endl;
    }
}

struct ComRem
{
    public:
        enum
        {
            // undefined/default state
            CT_UNDEF,
            // a single forward slash
            CT_FWDSLASH,
            // a single backward slash
            CT_BCKSLASH,
            // a single open parenthesis
            CT_OPENPAREN,
            // a single close parenthesis
            CT_CLOSEPAREN,
            /*
            * C preprocessor line
            * (#((els?)if(n?def)?|else|include|import|warning|error|pragma|line))
            *
            * this was already a major hack in the ruby version (see old/main.rb),
            * and i'm not sure if it's worth implementing.
            */  
            CT_PREPROC,
            // a C++ comment (like this one!)
            CT_CPPCOMM,
            /* a C comment (like this one!) */
            CT_ANSICOMM,
            /*
            * a Pascal-style comment (* like this! *)
            * supporting '{' and '}' is going to be a bit tougher, i think.
            * according to freepascal (https://www.freepascal.org/docs-html/ref/refse2.html):
            *
            *   Remark: In TP and Delphi mode, nested comments are not allowed,
            *   for maximum compatibility with existing code for those compilers.
            *
            * so comment-nesting is supported-ish, but seriously only -ish.
            */ 
            CT_PASCALCOMM,
            // a hash-comment (python, perl, ruby, etc ...) # like this!
            // this will NOT WORK with many shell or perl scripts, as it
            // does not, nor ever will, support heredocs, et cetera.
            CT_HASHCOMM,
        };

        struct Options
        {
            //! is RemCom allowed to complain? default: yes
            bool use_warningmessages = true;

            //! is RemCom allowed to explain? default: no
            bool use_debugmessages = false;

            //! remove C++-style comments? default: yes
            bool allow_cppcomments = true;

            //! remove ANSI C comments? default: yes
            bool allow_ansicomments = true;

            //! remove Pascal-style comments? default: no
            bool allow_pascalcomments = false;

            //! handle #-style comments? default: no (they clash with the preprocessor!)
            bool allow_hashcomments = false;

            // if any preprocessor tokens were specified ...
            bool have_ppctokens = false;
            // and where they are stored.
            std::vector<std::string> ppctokens;

            // both infilename and outfilename are only used for diagnostics
            // and are only modified through m_infp and m_outfp, respectively.
            std::string infilename = "<stdin>";
            std::string outfilename = "<stdout>";
        };

    private:
        // parser options
        Options m_opts;

        // the input stream handle
        std::istream* m_infp;

        // the output stream handle
        std::ostream* m_outfp;

        // the previous character
        int m_prevch;

        // the current character
        int m_currch;

        // the next peeked character
        int m_peekch;

        // current state the parser is in
        int m_state;

        // current line the parser is looking at
        int m_posline;

        // current column the parser is looking at
        int m_poscol;
        
        // tracks comment nesting levels
        int m_pascalnest;

        // when encountering a '{' in pascalmode. 
        bool m_pascalbrace;

    private:
        void initdefaults()
        {
            m_state = CT_UNDEF;
            m_prevch = EOF;
            m_currch = EOF;
            m_peekch = EOF;
            m_posline = 1;
            m_poscol = 0;
            m_pascalnest = 0;
            m_pascalbrace = false;
        }

        template<typename... Args>
        void dbg(const std::string& fmtstr, Args&&... args)
        {
            if(m_opts.use_debugmessages)
            {
                Util::sfprintf(std::cerr, ">>>>[%s:%d:%d]: ", m_opts.infilename, m_posline, m_poscol);
                Util::sfprintf(std::cerr, fmtstr, args...);
                std::cerr << std::endl;
            }
        }

        template<typename... Args>
        void warn(const std::string& fmtstr, Args&&... args)
        {
            if(m_opts.use_warningmessages)
            {
                Util::sfprintf(std::cerr, "WARNING: [%p:%d:%d]: ", m_opts.infilename, m_posline, m_poscol);
                Util::sfprintf(std::cerr, fmtstr, args...);
                std::cerr << std::endl;
            }
        }

        bool is_pascalcomm_begin()
        {
            return (
                (('(' == m_currch) && ('*' == m_peekch)) ||
                ('{' == m_currch)
            );
        }


    public:
        ComRem(const Options& opts, std::istream* infp, std::ostream* outfp):
            m_opts(opts), m_infp(infp), m_outfp(outfp)
        {
            initdefaults();
        }

        int more()
        {
            // store previous character
            m_prevch = m_currch;
            // get current character
            m_currch = m_infp->get();
            // store next character, without advancing stream
            m_peekch = peek();
            // ignore carriage returns
            if(m_currch == '\r')
            {
                return more();
            }
            if(m_currch == '\n')
            {
                m_posline++;
                m_poscol = 0;
            }
            m_poscol++;
            return m_currch;
        }

        int peek()
        {
            return m_infp->peek();
        }

        void out(int ch)
        {
            m_outfp->put(ch);
        }

        bool run()
        {
            int quote;
            int morech;
            int state3Col;
            long startLine;
            long startCol;
            bool escaped;
            bool endOfString;
            state3Col = -99;
            while(true)
            {
                m_prevch = m_currch;
                m_currch = more();
                if(m_currch == EOF)
                {
                    return true;
                }
                switch(m_state)
                {
                    case CT_UNDEF:
                        if('\'' == m_currch || '"' == m_currch)
                        {
                            /* Read string literal...
                               needed to properly catch comments in strings. */
                            quote = m_currch;
                            startLine = m_posline;
                            startCol = m_poscol;
                            escaped = false;
                            endOfString = false;
                            out(m_currch);
                            while(true)
                            {
                                if(endOfString == true)
                                {
                                    break;
                                }
                                morech = more();
                                if(morech == EOF)
                                {
                                    break;
                                }
                                switch(morech)
                                {
                                    case '\\':
                                        escaped = !escaped;
                                        break;
                                    case '\'':
                                    case '"':
                                        if(!escaped && quote == morech)
                                        {
                                            endOfString = true;
                                        }
                                        escaped = false;
                                        break;
                                    default:
                                        escaped = false;
                                        break;
                                }
                                out(morech);
                            }
                            if(EOF == morech)
                            {
                                warn("unexpected end-of-file while reading %s literal, starting on line %d, column %d",
                                    (('"' == m_currch) ? "string" : "char"),
                                    startLine,
                                    startCol
                                );
                                return false;
                            }
                            break;
                        }
                        else if('/' == m_currch)
                        {
                            m_state = CT_FWDSLASH;
                            break;
                        }
                        else if(m_opts.allow_hashcomments && (m_currch == '#'))
                        {
                            m_state = CT_HASHCOMM;
                            break;
                        }
                        if(m_opts.allow_pascalcomments && is_pascalcomm_begin())
                        {
                            dbg("begin pascalcomment");
                            if(m_currch == '{')
                            {
                                m_pascalbrace = true;
                            }
                            m_state = CT_PASCALCOMM;
                            break;
                        }
                        out(m_currch);
                        break;
                    case CT_FWDSLASH: /* 1 slash */
                        /*
                        * huge kludge for odd corner case:
                        */
                        /*/ <--- here.
                        * state3Col marks the source column in which a C-style
                        * comment starts, so that it can tell if star-slash inside a
                        * C-style comment is the end of the comment or is the weird corner
                        * case marked at the start of _this_ comment block.
                        * see also CT_ANSICOMM.
                        */
                        if(('*' == m_currch) && (m_opts.allow_ansicomments == true))
                        {
                            m_state = CT_ANSICOMM;
                            state3Col = m_poscol - 1;
                        }
                        else if(('/' == m_currch) && (m_opts.allow_cppcomments == true))
                        {
                            m_state = CT_CPPCOMM;
                        }
                        else
                        {
                            /* It wasn't a comment after all. */
                            m_state = CT_UNDEF;
                            out(m_prevch);
                            out(m_currch);
                        }
                        break;
                    /* C++ comment */
                    case CT_CPPCOMM:
                    case CT_HASHCOMM:
                        if('\n' == m_currch)
                        {
                            m_state = CT_UNDEF;
                            out(m_currch);
                        }
                        break;
                    case CT_ANSICOMM: /* C comment */
                        dbg("in ansicomment: currch=%q", char(m_currch));
                        if('/' == m_currch)
                        {
                            if('*' == m_prevch)
                            {
                                /* Corner case which breaks this: */
                                /*/ <-- slash there */
                                /* That shows up twice in a piece of 3rd-party
                                   code i use. */
                                /* And thus state3Col was introduced :/ */
                                if(m_poscol != (state3Col + 2))
                                {
                                    m_state = CT_UNDEF;
                                    state3Col = -99;
                                }
                            }
                        }
                        break;
                    case CT_PASCALCOMM:
                        dbg("in pascalcomment: currch=%q", char(m_currch));
                        if((('*' == m_prevch) && (')' == m_currch)) || (m_pascalbrace && (m_currch == '}')))
                        {
                            if(m_pascalnest == 0)
                            {
                                if(m_pascalbrace)
                                {
                                    m_pascalbrace = false;
                                }
                                dbg("end pascalcomment");
                                m_state = CT_UNDEF;
                            }
                            else
                            {
                                warn("in pascalcomment: unnesting from level %d", m_pascalnest);
                                m_pascalnest--;
                            }
                        }
                        else if((('(' == m_currch) && (m_peekch == '*')) || (m_pascalbrace && (m_currch == '{')))
                        {
                            m_pascalnest += 1;
                            warn("in pascalcomment: nested comment level %d detected! this may likely break", m_pascalnest);

                        }
                        break;
                    default:
                        assert(!"impossible!");
                        break;
                }
                if('\n' == m_currch)
                {
                    state3Col = -99;
                }
            }
            return true;
        }


};

int main(int argc, char** argv)
{
    bool rc;
    bool have_infile;
    bool have_outfile;
    std::string tmpfn;
    std::istream* infp;
    std::ostream* outfp;
    ComRem::Options opts;
    infp = &std::cin;
    outfp = &std::cout;
    have_infile = false;
    have_outfile = false;
    /* no options yet */

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
    #if 1
    prs.on({"-x?", "--preprocessor=?"}, "remove comma-separated C-preprocessor tokens (i.e., '-xinclude,import')",
    [&](const OptionParser::Value& val)
    {
        // -x can be specified several times!
        Util::split(val.str(), ",", [&](const std::string& tok)
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
            opts.outfilename = pos[1];
            outfp = new std::ofstream(opts.outfilename, std::ios::out | std::ios::binary);
            if(!outfp->good())
            {
                Util::error("cannot open '%s' for writing", opts.outfilename);
                return 1;
            }
            have_outfile = true;
        }

    }
    catch(std::runtime_error& ex)
    {
        std::cerr << "error: " << ex.what() << std::endl;
    }
    ComRem x(opts, infp, outfp);
    rc = x.run();
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
