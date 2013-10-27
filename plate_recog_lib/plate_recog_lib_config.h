#pragma once

#	if defined WIN32
#		ifdef PLATE_RECOG_LIB
#			define PLATE_RECOG_EXPORT __declspec( dllexport )
#		else
#			define PLATE_RECOG_EXPORT __declspec( dllimport )
#		endif //PLATE_RECOG_LIB
#	else
#		define PLATE_RECOG_EXPORT
#	endif // WIN32
