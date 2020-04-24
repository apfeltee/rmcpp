
#pragma once
#include <functional>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>

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

    template<typename CharT>
    void split(
        const std::basic_string<CharT>& str,
        const std::basic_string<CharT>& delim,
        std::function<void(const std::basic_string<CharT>&)> cb)
    {
        size_t prevpos;
        size_t herepos;
        std::basic_string<CharT> token;
        prevpos = 0;
        herepos = 0;
        while(true)
        {
            herepos = str.find(delim, prevpos);
            if(herepos == std::basic_string<CharT>::npos)
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

class CommentStripper
{
    public:
        enum State
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
            bool remove_cppcomments = true;

            //! remove ANSI C comments? default: yes
            bool remove_ansicomments = true;

            //! remove Pascal-style comments? default: no
            bool remove_pascalcomments = false;

            //! handle #-style comments? default: no (they clash with the preprocessor!)
            bool remove_hashcomments = false;

            bool do_convertcpp = false;

            // infilename is only used for diagnostics
            std::string infilename = "<stdin>";
        };

        using OnCommentCallback = std::function<bool(State, char)>;

    private:
        // parser options
        Options m_opts;

        // the input stream handle
        std::istream* m_infp;

        // current state the parser is in
        State m_state;
        
        // the previous character
        int m_prevch;

        // the current character
        int m_currch;

        // the next peeked character
        int m_peekch;

        // current line the parser is looking at
        int m_posline;

        // current column the parser is looking at
        int m_poscol;
        
        // tracks comment nesting levels
        int m_pascalnest;

        // when encountering a '{' in pascalmode. 
        bool m_pascalbrace;

        // true if we're inside a comment
        bool m_incomment;

        OnCommentCallback m_oncommentcb;

    private:
        void initdefaults();

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

        bool is_pascalcomm_begin();

        void forward_comment(State st, char ch);
        void forward_comment(State st, const std::string& str);

    public:
        /*
        * infp has to be a pointer - a reference to the input stream.
        * i.e.,
        *
        *   CommentStripper cs(..., &std::cin);
        *
        * would create CommentStripper with standard input as input stream, whereas
        *
        *   std::ifstream ifs("somefile.cpp", std::ios::in | std::ios::binary);
        *   if(ifs.good())
        *   {
        *       CommentStripper ifs(..., &ifs);
        *       // alternatively, if CommentStripper is needed as a ref somewhere else:
        *       //csptr = new CommentStripper(...);
        *   }
        *
        * in other words, the input-stream does NOT need to be new'd.
        */
        CommentStripper(const Options& opts, std::istream* infp);

        /**
        * populates m_currchar with the current character in the stream cursor,
        * m_prevchar with the prior value of m_currchar, and m_peekch with the
        * value returned from peek().
        * also advances m_posline and m_poscol, as needed.
        * automatically discards carriage-returns.
        */
        int more();

        /**
        * @returns the next character in the input-stream, without advancing
        * the stream
        */
        int peek();

        void onComment(OnCommentCallback cb);

        /**
        * @param outfp the std::ostream-compatible output-stream to write to.
        * @returns true if no errors occured, false otherwise.
        */
        bool run(std::ostream& outfp);
};

