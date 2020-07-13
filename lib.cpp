
/**
* based on stripcomments.c, written by Stephan Beal (stephan@wanderinghorse.net),
* which is in the public domain. Thanks mate.
*/

/*
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
*/

#include <cassert>
#include "rmcpp.h"

void CommentStripper::initdefaults()
{
    m_state = CT_UNDEF;
    m_prevch = EOF;
    m_currch = EOF;
    m_peekch = EOF;
    m_posline = 1;
    m_poscol = 0;
    m_pascalnest = 0;
    m_pascalbrace = false;
    m_incomment = false;
}

bool CommentStripper::is_space()
{
    if(m_currch == '\n')
    {
        if((m_peekch == '\r') || (m_peekch == '\n'))
        {
            return true;
        }
        return false;
    }
    return (
        (m_currch == ' ') ||
        (m_currch == '\t')
    );
}

void CommentStripper::do_skipemptylines()
{
    while(true)
    {
        if((m_currch == '\n') && ((m_peekch == '\r') || (m_peekch == '\n')))
        {
            m_currch = more();
        }
        else
        {
            return;
        }
    }
}

bool CommentStripper::is_pascalcomm_begin()
{
    return (
        /*
        * pascal has two kinds of block comments:
        *
        *   (* traditional comments, derived from modula *)
        *
        * and
        *
        *   { these things }
        *
        * both are valid, although the ISO standard only talks about (* these *).
        * since they can be nested, they also need to be tracked (see CT_PASCALCOMM).
        */
        (('(' == m_currch) && ('*' == m_peekch)) ||
        ('{' == m_currch)
    );
}

CommentStripper::CommentStripper(const Options& opts, std::istream* infp):
    m_opts(opts), m_infp(infp)
{
    initdefaults();
}

int CommentStripper::more()
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

int CommentStripper::peek()
{
    return m_infp->peek();
}

void CommentStripper::forward_comment(State st, char ch)
{
    if(m_oncommentcb)
    {
        if(!m_oncommentcb(st, ch))
        {
            m_oncommentcb = nullptr;
        }
    }
}

void CommentStripper::forward_comment(State st, const std::string& str)
{
    for(char ch: str)
    {
        forward_comment(st, ch);
    }
}

void CommentStripper::onComment(OnCommentCallback cb)
{
    m_oncommentcb = cb;
}

bool CommentStripper::run(std::ostream& outfp)
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
                    outfp.put(m_currch);
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
                        outfp.put(morech);
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
                else if(m_opts.remove_hashcomments && (m_currch == '#'))
                {
                    m_state = CT_HASHCOMM;
                    m_incomment = true;
                    break;
                }
                else if(m_opts.remove_pascalcomments && is_pascalcomm_begin())
                {
                    dbg("begin pascalcomment");
                    if(m_currch == '{')
                    {
                        m_pascalbrace = true;
                    }
                    m_state = CT_PASCALCOMM;
                    m_incomment = true;
                    break;
                }
                else if(is_space() && m_opts.remove_emptylines)
                {
                    do_skipemptylines();
                    if(m_currch == EOF)
                    {
                        return false;
                    }
                }
                else
                {
                    dbg("???how did we end up here???");
                }
                outfp.put(m_currch);
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
                if(('*' == m_currch) && (m_opts.remove_ansicomments == true))
                {
                    m_state = CT_ANSICOMM;
                    m_incomment = true;
                    state3Col = m_poscol - 1;
                    forward_comment(m_state, "/*");
                    break;
                }
                if(('/' == m_currch) && ((m_opts.remove_cppcomments || m_opts.do_convertcpp)))
                {
                    m_state = CT_CPPCOMM;
                    m_incomment = true;
                    if(m_opts.do_convertcpp)
                    {
                        outfp << "/*";
                    }
                    else
                    {
                        forward_comment(m_state, "//");
                    }
                    break;
                }
                /* It wasn't a comment after all. */
                m_state = CT_UNDEF;
                m_incomment = false;
                outfp.put(m_prevch);
                outfp.put(m_currch);
                
                break;
            /* C++ comment */
            case CT_CPPCOMM:
            case CT_HASHCOMM:
                forward_comment(m_state, m_currch);
                if('\n' == m_currch)
                {
                    if((m_state == CT_CPPCOMM) && (m_opts.do_convertcpp == true))
                    {
                        outfp << "*/";
                    }
                    m_state = CT_UNDEF;
                    m_incomment = false;
                    outfp.put(m_currch);
                    forward_comment(CT_UNDEF, 0);
                }
                else if(m_incomment && m_opts.do_convertcpp)
                {
                    outfp.put(m_currch);
                }
                break;
            case CT_ANSICOMM: /* C comment */
                dbg("in ansicomment: currch=%q", char(m_currch));
                forward_comment(m_state, m_currch);
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
                            m_incomment = false;
                            m_state = CT_UNDEF;
                            state3Col = -99;
                            forward_comment(CT_UNDEF, 0);
                        }
                    }
                }
                break;
            case CT_PASCALCOMM:
                dbg("in pascalcomment: currch=%q", char(m_currch));
                forward_comment(m_state, m_currch);
                if((('*' == m_prevch) && (')' == m_currch)) || (m_pascalbrace && (m_currch == '}')))
                {
                    if(m_pascalnest == 0)
                    {
                        if(m_pascalbrace)
                        {
                            m_pascalbrace = false;
                        }
                        dbg("end pascalcomment");
                        m_incomment = false;
                        m_state = CT_UNDEF;
                        forward_comment(m_state, 0);
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

