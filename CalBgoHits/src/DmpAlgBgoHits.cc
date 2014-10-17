/* for BGO Hits                           */
/* yong zhzhy@mail.ustc.edu.cn 08-10-2014 */

#include "DmpAlgBgoHits.h"
#include "DmpDataBuffer.h"
#include "TMath.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "DmpBgoBase.h"
#include "DmpCore.h"
#include <iostream>
#include <fstream>
//-------------------------------------------------------------------
DmpAlgBgoHits::DmpAlgBgoHits()
 :DmpVAlg("BgoHits"),
	fEvtHeader(0),
	fBgoRaw(0),
	fBgoDyCoe(0),
	fBgoMips(0),
  //      fBgoAtt(0),
	fBgoHits(0)
{
        
 }

//-------------------------------------------------------------------
bool DmpAlgBgoHits::GetDyCoePar(){
  TFile *fDyPar=new TFile("./DyCoe/DyCoePar.root");
  if(fDyPar==0){
    std::cout<<"Can not open DyCoe Par root file!"<<std::endl;
    return false;
  }
  TTree *Dytree=(TTree*)fDyPar->Get("Calibration/Bgo");
  TBranch *b_fBgoDyCoe;
  Dytree->SetBranchAddress("DyCoe",&fBgoDyCoe,&b_fBgoDyCoe);
  Dytree->GetEntry(0);
  //prepare parameters
  short gid=0,l=0,b=0,s=0;
  short nPmts=(short)fBgoDyCoe->GlobalPMTID.size();
  for(short i=0;i<nPmts; ++i){
    gid=fBgoDyCoe->GlobalPMTID[i];
    l=DmpBgoBase::GetLayerID(gid);
    b=DmpBgoBase::GetBarID(gid);
    s=DmpBgoBase::GetSideID(gid);
    DyCoePar_58[l][b][s][0]=fBgoDyCoe->Slp_Dy8vsDy5[i];
    DyCoePar_58[l][b][s][1]=fBgoDyCoe->Inc_Dy8vsDy5[i];
    DyCoePar_25[l][b][s][0]=fBgoDyCoe->Slp_Dy5vsDy2[i];
    DyCoePar_25[l][b][s][1]=fBgoDyCoe->Inc_Dy5vsDy2[i];
  }
  delete Dytree;
  delete fDyPar;
  //usage: QdcCoe[fGidOrder[gid]];//Quadratic Coefficients
  //       Slope[...],Cst[...] are same.
  return true;
}
//-------------------------------------------------------------------
bool DmpAlgBgoHits::GetMipsPar(){
  TFile *fMipPar=new TFile("./MIPs/MIPsPar.root");
  if(fMipPar==0){
    std::cout<<"Can not open MIPs Par root file!"<<std::endl;
    exit(0);
  }
  TTree *Miptree=(TTree*)fMipPar->Get("Calibration/Bgo");
  TBranch *b_fBgoMip;
  Miptree->SetBranchAddress("Mips",&fBgoMips,&b_fBgoMip);
  Miptree->GetEntry(0);
  //prepare parameters
  short gid=0,l=0,b=0,s=0;
  short nBars=(short)fBgoMips->GlobalBarID.size();
  for(short i=0;i<nBars;++i){
    gid=fBgoMips->GlobalBarID[i];
    l=DmpBgoBase::GetLayerID(gid);
    b=DmpBgoBase::GetBarID(gid);
    s=fBgoMips->BgoSide[i];//s=0,1,2
    MipsPar[l][b][s][0]=fBgoMips->MPV[i]+0.222783*fBgoMips->Lwidth[i];//corrected MPV
    MipsPar[l][b][s][1]=fBgoMips->Gsigma[i];
    MipsPar[l][b][s][2]=fBgoMips->Lwidth[i];
  }
  delete Miptree;
  delete fMipPar;
  //usage: QdcCoe[fGidOrder[gid]];//Quadratic Coefficients
  //       Slope[...],Cst[...] are same.
  return true;
} 
//-------------------------------------------------------------------
bool DmpAlgBgoHits::GetAttPar(){
  ifstream Apar;
  Apar.open("./Attenuation/AttPar");
   if(!Apar.good()){
    std::cout<<"Can not open Att Par file!"<<std::endl;
    return false;
  } 
  
  short l=0,b=0;
  double attpar;
  short nGbar=14*22;
   for(short i=0; i<nGbar;i++){
     l=(short)(i/22);
     b=i%22;
     for(int j=0 ;j<2;j++){ 
      Apar>>attpar;
      AttPar[l][b][j]=attpar;
//      std::cout<<AttPar[i][j]<<"\t";
    }
    std::cout<<std::endl;
  }
  Apar.close();
  return true;
}   
//-------------------------------------------------------------------
DmpAlgBgoHits::~DmpAlgBgoHits(){
}

//-------------------------------------------------------------------
bool DmpAlgBgoHits::Initialize(){
 // gRootIOSvc->Set("Output/FileName","./"+gRootIOSvc->GetInputFileName()+"_Hits.root");
  // read input data
  fEvtHeader = dynamic_cast<DmpEvtHeader*>(gDataBuffer->ReadObject("Event/Cutped/EventHeader"));
  fBgoRaw = dynamic_cast<DmpEvtBgoRaw*>(gDataBuffer->ReadObject("Event/Cutped/Bgo"));
  fBgoHits = new DmpEvtBgoHits();
  gDataBuffer->RegisterObject("Event/Cal/Hits",fBgoHits,"DmpEvtBgoHits");
        
  
        bool prepareDyCoePar=GetDyCoePar();
	if(!prepareDyCoePar){
	  std::cout<<"Error:Can not read DyCoe Par!"<<std::endl;
	  return false;
	}
         
        bool prepareMipPar=GetMipsPar();
	if(!prepareMipPar){
	  std::cout<<"Error:Can not read Mips Par!"<<std::endl;
	  return false;
	}
        bool prepareAttPar=GetAttPar();
	if(!prepareAttPar){
	  std::cout<<"Error:Can not read Att Par!"<<std::endl;
	  return false;
	}

  return true;
}
//-------------------------------------------------------------------
bool DmpAlgBgoHits::Reset(){
Position.SetXYZ(0.,0.,0.);

}
//-------------------------------------------------------------------
bool DmpAlgBgoHits::ProcessThisEvent(){
 //Reset event class
 fBgoHits->Reset();
 Reset();
 //choose dynodes: 2,5 or 8
  double HitsBuffer[14][22][2];
  short  tag[14][22][2];
  memset(tag,0,sizeof(tag));
  memset(HitsBuffer,0,sizeof(HitsBuffer));
  short nSignal=fBgoRaw->fGlobalDynodeID.size();
  short gid=0,l=0,b=0,s=0,d=0;
  double adc=0.;
  for(short i=0;i<nSignal;++i){ 
    gid=fBgoRaw->fGlobalDynodeID[i];
    adc=fBgoRaw->fADC[i];
    l=DmpBgoBase::GetLayerID(gid);
    b=DmpBgoBase::GetBarID(gid);
    s=DmpBgoBase::GetSideID(gid);
    d=DmpBgoBase::GetDynodeID(gid);
    if(b>=22){continue;}//spare channels

    if(d==8&&adc<10000){
      HitsBuffer[l][b][s]=adc/MipsPar[l][b][s][0];
      tag[l][b][s]=d;
    }  
    else if(d==5&&adc>200&&adc<8000&&tag[l][b][s]!=8){
      double adc_8=adc*DyCoePar_58[l][b][s][0]+DyCoePar_58[l][b][s][1];
      HitsBuffer[l][b][s]=adc_8/MipsPar[l][b][s][0];
      tag[l][b][s]=d;
    }
    else if(d==2&&adc>200&&tag[l][b][s]!=5&&tag[l][b][s]!=8){
      double adc_5=adc*DyCoePar_25[l][b][s][0]+DyCoePar_25[l][b][s][1];
      double adc_8=adc_5*DyCoePar_58[l][b][s][0]+DyCoePar_58[l][b][s][1];
      HitsBuffer[l][b][s]=adc_8/MipsPar[l][b][s][0];
      tag[l][b][s]=d;
    }
  }
  //fill Hits event class
  for(short il=0;il<14;++il){
    for(short ib=0;ib<22;++ib){
      //s0*s1 !=0;
      if(HitsBuffer[il][ib][0]!=0&&HitsBuffer[il][ib][1]!=0){
      short gid_bar=DmpBgoBase::ConstructGlobalBarID(il,ib);
      fBgoHits->fGlobalBarID.push_back(gid_bar);
      fBgoHits->fES0.push_back(HitsBuffer[il][ib][0]*22.5);
      fBgoHits->fES1.push_back(HitsBuffer[il][ib][1]*22.5);
        if(TMath::Abs(HitsBuffer[il][ib][0]/HitsBuffer[il][ib][1]-1)<0.4){//1-exp(-600/lambda)=0.36; (lambda=1350mm)
          if(HitsBuffer[il][ib][0]*HitsBuffer[il][ib][1]*MipsPar[il][ib][0][0]*MipsPar[il][ib][1][0]<0){
	DmpLogError<<"Hits0= "<<HitsBuffer[il][ib][0]<<" Hits1= "<<HitsBuffer[il][ib][1]<<" Mip_MPV[0]="<<MipsPar[il][ib][0][0]<<" Mip_MPV[1]"<<MipsPar[il][ib][1][0]<<DmpLogEndl;
	  }
          double combinedhits=TMath::Sqrt(HitsBuffer[il][ib][0]*HitsBuffer[il][ib][1]*MipsPar[il][ib][0][0]*MipsPar[il][ib][1][0])/MipsPar[il][ib][2][0]; 
          DmpLogWarning<<"Layer="<<il<<" Bar="<<ib<<" Event="<<gCore->GetCurrentEventID()<<" combinedhits:"<<combinedhits<<" ADC"<<" MipsPar:"<<MipsPar[il][ib][2][0]<<DmpLogEndl;
          fBgoHits->fEnergy.push_back(combinedhits*22.5);
	  double pos=(TMath::Log(HitsBuffer[il][ib][0]/HitsBuffer[il][ib][1])-AttPar[il][ib][1])/AttPar[il][ib][0];
	  if(il%2==0){
            Position.SetX(pos);
	  } 
	  else{
	    Position.SetY(pos);
	  }
          fBgoHits->fPosition.push_back(Position);//along the BGO bar (mm)
        }
        else  {
          DmpLogWarning<<"Layer="<<il<<" Bar="<<ib<<" Event="<<gCore->GetCurrentEventID()<<" Energies from two BGO sides UNmatch!"<<DmpLogEndl;
        //std::cout<<"Side 0:"<<HitsBuffer[il][ib][0]*22.5<<" MeV;\n"<<"Side 1:"<<HitsBuffer[il][ib][1]*22.5<<" MeV"<<std::endl;          
          fBgoHits->fEnergy.push_back(HitsBuffer[il][ib][0]*22.5);
          fBgoHits->fPosition.push_back(Position);//along the BGO bar (mm)
        }
      }
      //s0!=0 or s1 !=0;
  //    else if(HitsBuffer[il][ib][0]!=0||HitsBuffer[il][ib][1]!=0){
    //    if((HitsBuffer[il][ib][0]+HitsBuffer[il][ib][1])<0.5){//set noise threshold: 0.5MIPs
    //    std::cout<<"Noise hits removed (<0.5Mips on a single BGO side (0 on the other side))!"<<std::endl;
    //    }
   //     else {std::cout<<"Warning: Energies unmatch! (>0.5 MIPs one Side, 0 MIPs on the other side!);"<<std::endl;
   //     } 
   //   }
    }
  }
  return true;
}

//-------------------------------------------------------------------
bool DmpAlgBgoHits::Finalize(){
  return true;
}

