% -*- coding:utf-8; mode:latex; -*- %

% Copyright (C) 2018 Moritz Klammler <moritz.klammler@student.kit.edu>
%
% Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
% provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{graphstudy}[2018/02/28 LaTeX2e Class for Moritz Klammler's Master's Thesis]

\RequirePackage{etoolbox}
\RequirePackage{ifdraft}
\RequirePackage{ifluatex}

\DeclareOption*{\PassOptionsToClass{\CurrentOption}{scrbook}}
\ProcessOptions\relax

\LoadClass[%
  a4paper, 11pt, titlepage, cleardoublepage=empty, toc=listof, toc=index, toc=bibliography,
  \ifbool{luatex}{\directlua{tex.sprint(kpse.var_value("KOMA_OPTIONS") or "")}}{}
]{scrbook}

\InputIfFileExists{graphstudy.cfg}{\ClassInfo{graphstudy}{Loading configuration file config.tex ...}}{}

% KOMA does not use a global boolean to store whether it is in draft mode and the `ifdraft' package only detects the
% `draft' or `final' options correctly if they are passed to the top-level class.  But we also want to be able to select
% between draft and final mode at a later point.  So here is an ugly hack...
\ifdim\overfullrule=\z@\relax\@draftfalse\else\@drafttrue\fi

\RequirePackage{fontspec}
\RequirePackage{polyglossia}
\RequirePackage{microtype}
\RequirePackage{csquotes}
\RequirePackage[
  alldates = iso8601,
  backend = biber,
  defernumbers = true,
  firstinits = true,
  sortcites = true,
]{biblatex}
\RequirePackage{makeidx}
\RequirePackage[only-used = false, sort = true, hyperref = true, tooltip = false]{acro}
\RequirePackage{tikz}
\usetikzlibrary{calc,positioning}
\RequirePackage{enumitem}
\RequirePackage{multicol}
\RequirePackage{ragged2e}
\RequirePackage{amsmath,amssymb,bm}
\RequirePackage[figure]{algorithm2e}
\RequirePackage{listings}
\RequirePackage{datetime}
\RequirePackage[pdfencoding=unicode, psdextra, hidelinks, pdfpagelayout=TwoPageLeft]{hyperref}

\setmainlanguage{english}
\setotherlanguage[spelling=new]{german}
\setotherlanguage[variant=classic]{latin}

% This seems to be necessary even if using the `toc=bibliography` KOMA option.
\defbibheading{bibliography}[\bibname]{\addchap{#1}}

\newenvironment{extraleading}[1][1.25]{%
  \linespread{#1}\selectfont
}{%
}

%% Here we do the dirtiest thing ever, abusing the 'table' mechanism for our 'theorems' but it works and I simply cannot
%% be bothered to waste any more time trying to get a consistent style of the "list of X" indices.
\let\table\@undefined
\let\endtable\@undefined
\let\listoftheorems\listoftables
\AtBeginDocument{%
  \renewcaptionname{english}{\listfigurename}{List of Algorithms, Figures and Tables}
  \renewcaptionname{english}{\listtablename}{List of Definitions and Theorems}
}

\newenvironment{Figure}[1][]{%
  \renewcaptionname{english}{\figurename}{Figure}%
  \begin{figure}[#1]
}{%
  \end{figure}
}

\newenvironment{Table}[1][]{%
  \renewcaptionname{english}{\figurename}{Table}%
  \begin{figure}[#1]
}{%
  \end{figure}
}

\newenvironment{Algorithm}[1][]{%
  \renewcaptionname{english}{\figurename}{Algorithm}%
  \begin{algorithm}[#1]
}{%
  \end{algorithm}
}

\newenvironment{my@theorem}[2][\relax]{%
  \refstepcounter{table}%
  \ifx\relax#1\relax\else%
    \addcontentsline{lot}{section}{\protect\numberline{\thetable} #1}%
  \fi%
  \par\medskip\noindent\leavevmode\mythmfont%
  {\mythmtitlefont#2~\thetable\ifx\relax#1\relax\else\space(#1)\fi:\hskip0.5em plus 1em\linebreak[2]}%
  \ignorespaces%
}{%
  \par\medskip%
}

\newenvironment{definition}[1][\relax]{%
  \begin{my@theorem}[#1]{Definition}\ignorespaces%
}{%
  \end{my@theorem}%
}

\newcommand*{\mythmfont}{\slshape}
\newcommand*{\mythmtitlefont}{\scshape}

\newlist{acronyms}{description}{1}
\setlist[acronyms]{%
  before = \RaggedRight,
  font = \normalfont,
  itemindent = 0cm,
  labelindent = 0em,
  leftmargin = 5em,
  nolistsep,
  style = sameline,
}

\DeclareAcroListStyle{mystyle}{list}{list=theacronyms}
\acsetup{list-style=mystyle}

\newenvironment{theacronyms}{%
  \setlength{\columnsep}{2em}
  \begin{multicols}{2}
    \begin{acronyms}
}{%
    \end{acronyms}
  \end{multicols}
}

\DontPrintSemicolon
\SetAlCapSkip{3ex}
\SetAlgoHangIndent{2em}
\SetAlgoLined
\SetAlgoLongEnd
\SetAlgoSkip{bigskip}
\SetCommentSty{textit}
\SetFuncSty{textsc}
\SetKwSty{textsc}
\let\AlCapSty\mythmtitlefont
\setlength{\algomargin}{0em}

\setcounter{secnumdepth}{3}
\setcounter{tocdepth}{3}

\setlength{\parindent}{0em}
\setlength{\parskip}{1.5ex plus 2.5ex minus 0.5ex}

\newenvironment{abstract}[2][\abstractname]{%
  \begin{otherlanguage}{#2}
    \begin{center}
      \usekomafont{disposition}#1
    \end{center}
    \par\medskip
}{%
  \end{otherlanguage}
  \par\bigskip
}

\newfontfamily{\logofont}[
% URL = https://fonts.google.com/specimen/Roboto,
  Extension = .ttf,
  Path = fonts/,
  UprightFont = *-Regular,
  BoldFont = *-Bold,
  Ligatures = TeX,
]{Roboto}

\input{titlepage.tex}
\let\maketitle\makemytitle

\setcapindent{0em}
\setkomafont{captionlabel}{\mythmtitlefont}

\setkomafont{subject}{\logofont\Large}
\setkomafont{title}{\logofont\Huge}
\setkomafont{subtitle}{\logofont\Large}
\setkomafont{author}{\logofont\huge}
\setkomafont{disposition}{\bfseries}

\lstset{
  language = {},
  basicstyle = \normalfont\ttfamily\footnotesize,
}
