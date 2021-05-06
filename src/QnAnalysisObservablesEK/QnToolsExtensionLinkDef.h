//
// Created by eugene on 04/05/2021.
//

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class Qn::SystematicError +;
#pragma link C++ class Qn::DataContainer < Qn::SystematicError, Qn::Axis < double>> + ;

#pragma linl C++ class Qn::Dv1Dy + ;
#pragma link C++ class Qn::DataContainer < Qn::Dv1Dy, Qn::Axis < double>> + ;

#endif