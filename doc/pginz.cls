\ProvidesClass{pginz}
 
\newcommand{\headlinecolor}{\normalcolor}
\LoadClass[10pt,a4paper,twoside]{report}

\RequirePackage[a4paper,inner=3.5cm,outer=2.5cm,top=2.5cm,bottom=2.5cm]{geometry}
\linespread{1.3} %interlinia 1.5
\setlength{\parindent}{1.25cm}

\RequirePackage[utf8]{inputenc}
\RequirePackage[main=polish,english]{babel}
\RequirePackage{pdfpages}

\RequirePackage[titletoc,title]{appendix}
\RequirePackage{titlesec}
\RequirePackage{helvet}
\RequirePackage{siunitx}
\renewcommand*{\familydefault}{\sfdefault}

%*************
\RequirePackage{newtxtext, newtxmath} %lepiej wyglądające nagłówki
\RequirePackage{epstopdf} %do dołączania obrazków w formacie eps
\RequirePackage{hyperref} % hiperłącza wewnętrzne (cytowania, odnośniki do obrazków, równań)
\RequirePackage{xcolor,listings} %listingi 
\RequirePackage[font=small,labelfont=bf]{caption} %ustawienie czcionki 9pt na podpisach
\RequirePackage{cleveref} %odniesienia
\captionsetup[table]{justification=justified,singlelinecheck=false, format=hang} %ustawienie podpisów tabel 
\RequirePackage{enumitem} %to, czego brakowało przy symbolach
\RequirePackage[normalem]{ulem}
%*************

\titleformat{\chapter}[hang]
{\normalfont\fontsize{12}{15}\bfseries\MakeUppercase}{\thechapter.}{1em}{}
\titlespacing*{\chapter}{0pt}{12pt}{6pt}

\titleformat{\section}[hang]
{\normalfont\fontsize{10}{12}\bfseries\itshape}{\thesection.}{0.5em}{}
\titlespacing*{\section}{0pt}{12pt}{6pt}

\titleformat{\subsection}[hang]
{\normalfont\fontsize{10}{12}\itshape}{\thesubsection.}{0.5em}{}
\titlespacing*{\subsection}{0pt}{12pt}{6pt}

\crefformat{equation}{(#2#1#3)}

\renewcommand\familydefault{\sfdefault}

\definecolor{codegreen}{rgb}{0,0.6,0}
\definecolor{codegray}{rgb}{0.5,0.5,0.5}
\definecolor{codepurple}{rgb}{0.58,0,0.82}
\definecolor{backcolour}{rgb}{0.95,0.95,0.92}

\lstdefinestyle{mystyle}{
    backgroundcolor=\color{backcolour},   
    commentstyle=\color{codegreen},
    keywordstyle=\color{magenta},
    numberstyle=\tiny\color{codegray},
    stringstyle=\color{codepurple},
    basicstyle=\ttfamily\footnotesize,
    breakatwhitespace=false,         
    breaklines=true,                 
    captionpos=b,                    
    keepspaces=true,                 
    numbers=left,                    
    numbersep=5pt,                  
    showspaces=false,                
    showstringspaces=false,
    showtabs=false,                  
    tabsize=2
}

\lstset{style=mystyle}

\crefname{figure}{rys.}{rys.}
\Crefname{figure}{Rysunek}{Rysunki}
\crefname{table}{Tabela}{Tabele}
\Crefname{table}{Tabela}{Tabele}
\crefname{section}{Sekcja}{Sekcje}
\Crefname{section}{Sekcja}{Sekcje}
\crefname{chapter}{Rozdział}{Rozdziały}
\Crefname{chapter}{Rozdział}{Rozdziały}

\crefname{lstlisting}{Listing}{Listingi}
\Crefname{lstlisting}{Listing}{Listingi}

\RequirePackage{textcase}
\makeatletter
\let\oldcontentsline\contentsline
\def\contentsline#1#2{%
  \expandafter\ifx\csname l@#1\endcsname\l@chapter
    \expandafter\@firstoftwo
  \else
    \expandafter\@secondoftwo
  \fi
  {%
    \oldcontentsline{#1}{\MakeTextUppercase{#2}}%
  }{%
    \oldcontentsline{#1}{#2}%
  }%
}
\makeatother