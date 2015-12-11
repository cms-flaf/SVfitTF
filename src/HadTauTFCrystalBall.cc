#include "TauAnalysis/SVfitTF/interface/HadTauTFCrystalBall.h"

#include "FWCore/ParameterSet/interface/FileInPath.h"
#include "DataFormats/TauReco/interface/PFTau.h"

#include <iostream>
#include <string>
#include <assert.h> 
#include <math.h>

namespace
{
  std::string findFile(const std::string& fileName)
  {
    edm::FileInPath inputFile(fileName);
    if ( inputFile.fullPath() == "" ) {
      std::cerr << "Error: Cannot find file = " << fileName << " !!" << std::endl;
      assert(0);
    }
    return inputFile.fullPath().data();
  }
}

HadTauTFCrystalBall::HadTauTFCrystalBall(const std::string& inputFilePath)
  : cbParSize_(7),
    nDecayMode_(5),
    decayMode_(kAll),
    xx_(0),
    pp_(0),
    genPt_cache_(-1.),
    genEta_cache_(0.)
{ 
  //std::cout << "<HadTauTFCrystalBall::HadTauTFCrystalBall>:" << std::endl;

  xx_ = new double[1];
  pp_ = new double[cbParSize_];

  std::map<int, std::string> parDir;
  parDir[0] = inputFilePath + "/par0/";
  parDir[1] = inputFilePath + "/par1/";
  parDir[2] = inputFilePath + "/par2/";
  parDir[3] = inputFilePath + "/par3/";
  parDir[4] = inputFilePath + "/par4/";
  parDir[5] = inputFilePath + "/par5/";
  parDir[6] = inputFilePath + "/par6/";
  parDir[7] = inputFilePath + "/par7/";
  
  std::map<int, std::string> fileL2;
  fileL2[kAll]            = "TauJec11V1_L2Relative_AK5tauHPSlooseCombDBcorrAll.txt";
  fileL2[kOneProng0Pi0]   = "TauJec11V1_L2Relative_AK5tauHPSlooseCombDBcorrOneProng0Pi0.txt";
  fileL2[kOneProng1Pi0]   = "TauJec11V1_L2Relative_AK5tauHPSlooseCombDBcorrOneProng1Pi0.txt";
  fileL2[kOneProng2Pi0]   = "TauJec11V1_L2Relative_AK5tauHPSlooseCombDBcorrOneProng1Pi0.txt";
  fileL2[kThreeProng0Pi0] = "TauJec11V1_L2Relative_AK5tauHPSlooseCombDBcorrThreeProng0Pi0.txt";

  std::map<int, std::string> fileL3;
  fileL3[kAll]            = "TauJec11V1_L3Absolute_AK5tauHPSlooseCombDBcorrAll.txt";
  fileL3[kOneProng0Pi0]   = "TauJec11V1_L3Absolute_AK5tauHPSlooseCombDBcorrOneProng0Pi0.txt";
  fileL3[kOneProng1Pi0]   = "TauJec11V1_L3Absolute_AK5tauHPSlooseCombDBcorrOneProng1Pi0.txt";
  fileL3[kOneProng2Pi0]   = "TauJec11V1_L3Absolute_AK5tauHPSlooseCombDBcorrOneProng1Pi0.txt";
  fileL3[kThreeProng0Pi0] = "TauJec11V1_L3Absolute_AK5tauHPSlooseCombDBcorrThreeProng0Pi0.txt";

  for ( int decayMode = 0; decayMode < nDecayMode_; ++decayMode ){
    mapPar_[decayMode].resize(cbParSize_);
    for ( int par = 1; par < cbParSize_; ++par ){
      std::string l2JetParFileName = findFile(parDir[par] + fileL2[decayMode]); // each parameter has it own directory
      std::string l3JetParFileName = findFile(parDir[par] + fileL3[decayMode]);
      //std::cout << "reading tauES correction parameters for decayMode = " << decayMode << ", par #" << par << " from files:" << std::endl;
      //std::cout << " L2 = " << l2JetParFileName << std::endl;
      //std::cout << " L3 = " << l3JetParFileName << std::endl;
      mapPar_[decayMode][par] = new HadTauTFCrystalBallPar(decayMode, par, l2JetParFileName, l3JetParFileName);
    }
  }
}

HadTauTFCrystalBall::~HadTauTFCrystalBall()
{
  //std::cout << "<HadTauTFCrystalBall::~HadTauTFCrystalBall>:" << std::endl;

  delete [] xx_;
  delete [] pp_;

  for ( int decayMode = 0; decayMode < nDecayMode_; ++decayMode ){
    for ( int par = 0; par < cbParSize_; ++par ){
      delete mapPar_[decayMode][par];
    }
  }
} 

void HadTauTFCrystalBall::setDecayMode(int decayMode) const
{ 
  decayMode_ = decayMode; 

  int idxDecayMode = -1;  
  if      ( decayMode_ == reco::PFTau::kOneProng0PiZero   ) idxDecayMode = kOneProng0Pi0;
  else if ( decayMode_ == reco::PFTau::kOneProng1PiZero   ) idxDecayMode = kOneProng1Pi0;
  else if ( decayMode_ == reco::PFTau::kOneProng2PiZero   ) idxDecayMode = kOneProng2Pi0;
  else if ( decayMode_ == reco::PFTau::kThreeProng0PiZero ) idxDecayMode = kThreeProng0Pi0;
  else {
    std::cerr << "Warning: No transfer function defined for decay mode = " << decayMode_ << " !!"; 
    idxDecayMode = kAll;
  }
  assert(mapPar_.find(idxDecayMode) != mapPar_.end());

  thePar_.resize(cbParSize_);
  for ( int par = 1; par < cbParSize_; ++par ) {
    thePar_[par] = mapPar_.find(idxDecayMode)->second[par];
  }
}

namespace
{
  double fnc_dscb(double* xx, double* pp) 
  { 
    double x   = xx[0];
    // gaussian core
    //double N   = pp[0]; // norm
    double mu  = pp[1]; // mean
    double sig = TMath::Max(1.e-2, pp[2]); // variance
    // transition parameters
    double a1  = pp[3];
    double p1  = pp[4];
    double a2  = pp[5];
    double p2  = pp[6];
    //
    double u   = (x - mu)/sig;
    double A1  = pow(p1/fabs(a1), p1)*exp(-0.5*a1*a1);
    double A2  = pow(p2/fabs(a2), p2)*exp(-0.5*a2*a2);
    double B1  = p1/fabs(a1) - fabs(a1);
    double B2  = p2/fabs(a2) - fabs(a2);
    //
    double a1_plus_B1 = a1 + B1;
    std::cout << "a1 + B1 = " << a1_plus_B1 << ", mu + B1 = " << (mu + B1) << ", -1 + p1 = " << (-1. + p1) << std::endl;  
    double term1 = A1*TMath::Power(a1_plus_B1, -p1)*(a1_plus_B1*sig - TMath::Power(a1_plus_B1*sig, p1)*TMath::Power(mu + B1*sig, 1. - p1))/((-1. + p1)*sig);
    const double one_over_sqrtTwo = 1./TMath::Sqrt(2.);
    const double sqrtPi_over_two = TMath::Sqrt(0.5*TMath::Pi());    
    double term2 = sqrtPi_over_two*(TMath::Erf(a1*one_over_sqrtTwo) + TMath::Erf(a2*one_over_sqrtTwo));
    double a2_plus_B2 = a2 + B2;
    double term3 = A2*TMath::Power(a2_plus_B2, -p2)*(a2_plus_B2*sig - TMath::Power(a2_plus_B2*sig, p2)*TMath::Power(2. - mu + B2*sig, 1. - p2))/((-1. + p2)*sig);
    std::cout << "term1 = " << term1 << ", term2 = " << term2 << ", term3 = " << term3 << ", sig = " << sig << std::endl; 
    double one_over_N = term1 + term2 + term3;
    one_over_N *= sig; // CV: multiply result obtained from Mathematica by Jacobi factor dx/du = sig for variable transformation from x to u
    std::cout << "1/N = " << one_over_N << std::endl;
    double N = 1./one_over_N;
    //
    double result = 1.;
    if ( u < -a1 ) {
      std::cout << "B1 - u = " <<  (B1 - u) << std::endl;
      result = N*A1*pow(B1 - u, -p1);
    } else if ( u <  a2 ) {
      std::cout << "u = " << u << std::endl;	
      result = N*exp(-0.5*u*u);
    } else {
      std::cout << "B1 - u = " <<  (B1 - u) << std::endl;
      result = N*A2*pow(B2 + u, -p2);
    }
    return result;
  }
}

double HadTauTFCrystalBall::operator()(double recPt, double genPt, double genEta) const
{ 
  std::cout << "<HadTauTFCrystalBall::operator()>:" << std::endl;
  std::cout << " pT: rec = " << recPt << ", gen = " << genPt << std::endl;
  std::cout << " eta = " << genEta << std::endl;

  xx_[0] = recPt/genPt; 
  std::cout << " xx = " << xx_[0] << std::endl;

  if ( genPt != genPt_cache_ || genEta != genEta_cache_ ) {
    for ( int iPar = 1; iPar < cbParSize_; ++iPar ) {
      pp_[iPar] = (*thePar_[iPar])(genPt, genEta);
      pp_[iPar] = 1./(*thePar_[iPar])(genPt, genEta); // CV: temporary fix 
      std::cout << " pp(" << iPar << ") = " << pp_[iPar] << std::endl;
    }
    genPt_cache_ = genPt;
    genEta_cache_ = genEta;
  }

  double retVal = fnc_dscb(xx_, pp_);
  std::cout << "--> returning retVal = " << retVal << std::endl;	
  return retVal;
}


HadTauTFCrystalBall* HadTauTFCrystalBall::Clone(const std::string& label) const
{
  HadTauTFCrystalBall* clone = new HadTauTFCrystalBall();
  clone->cbParSize_ = cbParSize_;
  clone->nDecayMode_ = nDecayMode_;
  clone->decayMode_ = decayMode_;
  clone->xx_ = new double[1];
  clone->xx_[0] = xx_[0];
  clone->pp_ = new double[cbParSize_];
  for ( int iPar = 1; iPar < cbParSize_; ++iPar ) {
    clone->pp_[iPar] = pp_[iPar];
  }
  for ( std::map<int, vHadTauTFCrystalBallParPtr>::const_iterator entryPar = mapPar_.begin();
	entryPar != mapPar_.end(); ++entryPar ) {
    const vHadTauTFCrystalBallParPtr& cbPars = entryPar->second;
    vHadTauTFCrystalBallParPtr cbPars_cloned;
    for ( int iPar = 1; iPar < cbParSize_; ++iPar ) {  
      const HadTauTFCrystalBallPar* cbPar = cbPars[iPar];
      cbPars_cloned.push_back(new HadTauTFCrystalBallPar(*cbPar));
    }
    clone->mapPar_[decayMode_] = cbPars_cloned;
  }
  clone->setDecayMode(decayMode_);
  clone->genPt_cache_ = genPt_cache_;
  clone->genEta_cache_ = genEta_cache_;
  return clone;
}