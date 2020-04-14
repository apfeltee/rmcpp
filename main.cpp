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


#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include "../optionparser/optionparser.hpp"

namespace Util
{
    struct KnownEscape
    {
        int chval;
        const char* strval;
    };

    static const KnownEscape known_escapes[] =
    {
        {0, "\\0"},
        {1, "\\1"},
        {2, "\\2"},
        {3, "\\3"},
        {'\n', "\\n"},
        {'\t', "\\t"},
        {'\r', "\\r"},
        {'\f', "\\f"},
        {'\v', "\\v"},
        #if !defined(_MSC_VER)
        {'\e', "\\e"},
        #endif
        {0, NULL},
    };

    template<typename CharT>
    void escapechar(std::basic_ostream<CharT>& out, int ch, bool wquotes=false)
    {
        int i;
        // hack to write to wchar_t streams
        // despite the standard, this is actually completely legal.
        auto put = [&](const char* str)
        {
            int k;
            for(k=0; str[k]!=0; k++)
            {
                out.put(CharT(str[k]));
            }
        };
        if((ch >= CharT('A')) && (ch <= CharT('z')))
        {
            if(ch == '\\')
            {
                out.put(CharT('\\')).put(CharT('\\'));
            }
            else
            {
                out << CharT(ch);
            }
        }
        else
        {
            for(i=0; known_escapes[i].strval != NULL; i++)
            {
                if(ch == known_escapes[i].chval)
                {
                    put(known_escapes[i].strval);
                    return;
                }
            }
            if((ch == CharT('"')) && wquotes)
            {
                put("\\\"");
                return;
            }
            // still here? then it wasn't in known_escapes.
            out
                << CharT('\\')
                << CharT('x')
                << std::setfill('0')
                << std::setw(2)
                << std::uppercase
                << std::hex
                << CharT(ch)
            ;
        }
    }

    template<typename CharT>
    void escapestring(std::basic_ostream<CharT>& out, const std::basic_string<CharT>& str, bool wquotes=false)
    {
        if(wquotes)
        {
            out.put(CharT('"'));
        }
        for(CharT ch: str)
        {
            escapechar(out, ch, wquotes);
        }
        if(wquotes)
        {
            out.put(CharT('"'));
        }
    }

    template<typename CharT>
    std::basic_ostream<CharT>&
    sfprintf(
        std::basic_ostream<CharT>& out,
        std::string_view format)
    {
        out << format;
        return out;
    }
     
    template<typename CharT, typename Type, typename... Targs>
    std::basic_ostream<CharT>& sfprintf(
        std::basic_ostream<CharT>& out,
        std::string_view fmt,
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
                    case 'q':
                    case 'p':
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

}

struct ComRem
{
    public:
        enum
        {
            /*
                0==not in comment.
                1==1 slash,
                2==C++ comment,
                3==C comments
            */
            CT_UNDEF,
            CT_FWDSLASH,
            CT_BCKSLASH,
            CT_OPENPAREN,
            CT_CLOSEPAREN,
            CT_PREPROC,
            CT_CPPCOMM,
            CT_ANSICOMM,
            CT_PASCALCOMM,
            CT_HASHCOMM,
            
        };

        struct Options
        {
            bool debugmessages = false;
            bool cppcomments = true;
            bool ansicomments = true;
            bool pascalcomments = false;
            bool hashcomments = false;
            bool preprocessor = false;
        };

    private:
        Options m_opts;
        std::istream* m_infp;
        std::ostream* m_outfp;
        int m_morech;
        int m_prevch;
        int m_currch;
        int m_peekch;
        int m_state;
        int m_posline;
        int m_poscol;


    private:
        void initdefaults()
        {
            m_posline = 1;
            m_state = CT_UNDEF;
            m_poscol = 0;
            m_posline = 1;
            m_prevch = EOF;
        }

        template<typename... Args>
        void dbg(std::string_view fmtstr, Args&&... args)
        {
            if(m_opts.debugmessages)
            {
                /*
                fprintf(stderr, ">>>>[%d:%d]: ", m_posline, m_poscol);
                fprintf(stderr, fmtstr, args...);
                fprintf(stderr, "\n");
                */
                Util::sfprintf(std::cerr, ">>>>[%d:%d]: ", m_posline, m_poscol);
                Util::sfprintf(std::cerr, fmtstr, args...);
                std::cerr << std::endl;
            }
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
                                m_morech = more();
                                if(m_morech == EOF)
                                {
                                    break;
                                }
                                switch(m_morech)
                                {
                                    case '\\':
                                        escaped = !escaped;
                                        break;
                                    case '\'':
                                    case '"':
                                        if(!escaped && quote == m_morech)
                                        {
                                            endOfString = true;
                                        }
                                        escaped = false;
                                        break;
                                    default:
                                        escaped = false;
                                        break;
                                }
                                out(m_morech);
                            }
                            if(EOF == m_morech)
                            {
                                fprintf(stderr,
                                    "Unexpected EOF while reading %s literal on line %ld, column %ld.\n",
                                        ('\'' == m_currch) ? "char" : "string", startLine, startCol);
                                return false;
                            }
                            break;
                        }
                        else if('/' == m_currch)
                        {
                            m_state = CT_FWDSLASH;
                            break;
                        }
                        else if((('(' == m_currch) && ('*' == m_peekch)) && (m_opts.pascalcomments == true))
                        {
                            dbg("begin pascalcomment");
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
                        if(('*' == m_currch) && (m_opts.ansicomments == true))
                        {
                            m_state = CT_ANSICOMM;
                            state3Col = m_poscol - 1;
                        }
                        else if(('/' == m_currch) && (m_opts.cppcomments == true))
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
                    case CT_CPPCOMM: /* C++ comment */
                        if('\n' == m_currch)
                        {
                            m_state = CT_UNDEF;
                            out(m_currch);
                        }
                        break;
                    case CT_ANSICOMM: /* C comment */
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
                        // no support for nested comments.
                        dbg("in ct_pascalcomm: currch='%c'", m_currch);
                        if(')' == m_currch)
                        {
                            if('*' == m_prevch)
                            {
                                dbg("in ct_pascalcomm: end of comment");
                                m_state = CT_UNDEF;
                            }
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
    prs.on({"-d", "--debug"}, "show debug messages", [&]
    {
        opts.debugmessages = true;
    });
    prs.on({"-C", "--keepansi"}, "keep ansi C comments", [&]
    {
        opts.ansicomments = false;
    });
    prs.on({"-X", "--keepcpp"}, "keep C++ comments", [&]
    {
        opts.cppcomments = false;
    });
    prs.on({"-P", "--pascal"}, "also remove pascal style comments)", [&]
    {
        opts.pascalcomments = true;
    });
    try
    {
        prs.parse(argc, argv);
        auto pos = prs.positional();
        if(pos.size() > 0)
        {
            tmpfn = pos[0];
            infp = new std::ifstream(tmpfn, std::ios::in | std::ios::binary);
            if(!infp->good())
            {
                fprintf(stderr, "cannot open '%s' for reading", tmpfn.c_str());
                return 1;
            }
            have_infile = true;
        }
        if(pos.size() > 1)
        {
            tmpfn = pos[1];
            outfp = new std::ofstream(tmpfn, std::ios::out | std::ios::binary);
            if(!outfp->good())
            {
                fprintf(stderr, "cannot open '%s' for writing", tmpfn.c_str());
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
