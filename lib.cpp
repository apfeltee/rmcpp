
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
                    outfp.put(m_prevch);
                    outfp.put(m_currch);
                }
                break;
            /* C++ comment */
            case CT_CPPCOMM:
            case CT_HASHCOMM:
                if('\n' == m_currch)
                {
                    m_state = CT_UNDEF;
                    outfp.put(m_currch);
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

