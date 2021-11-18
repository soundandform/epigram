set VERSION=67
set BOOST=boost_1_%VERSION%_0

subst w: ..
cd w:\epigram

if not exist %BOOST%.zip powershell -command "start-bitstransfer -source https://boostorg.jfrog.io/artifactory/main/release/1.%VERSION%.0/source/%BOOST%.zip -destination %BOOST%.zip"
if not exist external\boost powershell -command "expand-archive %BOOST%.zip -destination external"
cd external
rename %BOOST% boost