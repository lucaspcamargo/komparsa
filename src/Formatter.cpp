#include "Formatter.h"

#include <QStringBuilder>
#include <QStringList>
#include <QDebug>
#include <algorithm>


QString gemtextToHTML( QString source )
{
    QString ret;
    
    QStringList lines = source.split(QStringLiteral("\n"));
    
    bool inlist = false;
    bool inpre = false;
    
    for(const QString &line: lines)
    {
        if (inlist && !line.startsWith(QStringLiteral("*")))
        {
            inlist = false;
            ret += QStringLiteral("</ul>");
        }
        
        if(line.startsWith(QStringLiteral("```")))
        {
            // pre toggle
            if(inpre)
            {
                ret += QStringLiteral("</pre>\n");
                inpre = false;
            }
            else
            {
                ret += QStringLiteral("<pre>\n");
                inpre = true;
            }
            continue;
        }
        else if(inpre)
        {
            ret += line;
            ret += QStringLiteral("\n");
            continue;
        }
        
        if(line.startsWith(QStringLiteral("*")))
        {
            // bullet point
            if(!inlist)
            {
                ret += QStringLiteral("<ul>\n");
                inlist = true;
            }
            
            ret += QStringLiteral("<li>");
            ret += line.sliced(1).trimmed();
            ret += QStringLiteral("</li>\n");
        }
        else if (line.startsWith(QStringLiteral("#")))
        {
            // header
            int hlevel = 1;
            if(line.startsWith(QStringLiteral("###")))
                hlevel = 3;
            else if(line.startsWith(QStringLiteral("##")))
                hlevel = 2;
            
            ret += QStringLiteral("<h%1>").arg(hlevel);
            ret += line.sliced(hlevel).trimmed();
            ret += QStringLiteral("</h%1>\n").arg(hlevel);
        }
        else if (line.startsWith(QStringLiteral(">")))
        {
            // quote
            ret += QStringLiteral("<blockquote>");
            ret += line.sliced(1).trimmed();
            ret += QStringLiteral("</blockquote>\n");
        }
        else if(line.startsWith(QStringLiteral("=>")))
        {
            // link
            QString trimmed = line.sliced(2).trimmed();
            if(trimmed.contains(QStringLiteral(" ")) || trimmed.contains(QStringLiteral("\t")))
            {
                // URL with firendly text
                auto firstSpace = trimmed.indexOf(QStringLiteral(" "));
                auto firstTab = trimmed.indexOf(QStringLiteral("\t"));
                auto toUse = firstSpace;
                if(firstTab >= 0 && firstSpace >= 0)
                    toUse = std::min(firstTab, firstSpace);
                else if(firstTab >= 0 && firstSpace < 0)
                    toUse = firstTab;
                QString url = trimmed.first(toUse);
                QString alt = trimmed.sliced(url.size()).trimmed();
                qDebug() << trimmed << "FS" << firstSpace << "URL" << url << "ALT" << alt;
                ret += QStringLiteral("<p><a href=\"%1\">%2 (%3)</a></p>\n").arg(url, alt, url);
            }
            else
            {
                // just a url
                ret += QStringLiteral("<p><a href=\"%1\"/></p>\n").arg(trimmed);
            }
        }
        else
        {
            // normal text line
            QString trimmed = line.trimmed();
            ret += QStringLiteral("<p>");
            ret += line.trimmed();
            ret += QStringLiteral("</p>\n");
        }
        
    }
    
    return ret;
    
}
