// Copyright (c) 1996-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#ifndef __WNT_Dword_HeaderFile
# include <WNT_Dword.hxx>
#endif  // __WNT_Dword_HeaderFile

const Handle( Standard_Type )& STANDARD_TYPE( WNT_Dword ) {

 static Handle( Standard_Type ) _aType = new Standard_Type (
                                              "WNT_Dword", sizeof ( WNT_Dword )
                                             );

 return _aType;

}  // end function
