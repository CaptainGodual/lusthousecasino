#pragma once
#include "CoreMinimal.h"

// Local-space routing for the authored venue, with pedestrian clearance baked into blockers.
struct FCasinoFloorBlocker
{
    FVector2D Center, Extent;
    float Yaw = 0;
    bool Contains(FVector2D P) const
    {
        const FVector2D D=(P-Center).GetRotated(-Yaw);
        return FMath::Abs(D.X)<=Extent.X && FMath::Abs(D.Y)<=Extent.Y;
    }
};
struct FCasinoFloorRoute
{
    TArray<FCasinoFloorBlocker> Blockers;
    static float Height(float Y) { return 41.f*FMath::Clamp((330.f-Y)/180.f,0.f,1.f); }
    bool Free(FVector2D P) const
    {
        if(P.X < -1050 || P.X > 1050 || P.Y < -440 || P.Y > 1650) return false;
        // The central stair/ramp is the only connection between floor and stage.
        if(P.Y>150 && P.Y<350 && FMath::Abs(P.X)>100) return false;
        for(const auto& B:Blockers) if(B.Contains(P)) return false;
        return true;
    }
    bool Segment(FVector2D A,FVector2D B) const
    {
        const int32 N=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(A,B)/8.f));
        for(int32 I=0;I<=N;++I) if(!Free(FMath::Lerp(A,B,float(I)/N))) return false;
        return true;
    }
    bool Plan(FVector Start,FVector Goal,TArray<FVector>& Out) const
    {
        Out.Reset(); const FVector2D S(Start.X,Start.Y),G(Goal.X,Goal.Y);
        if(!Free(S)||!Free(G)) return false;
        constexpr int32 NX=53,NY=53,N=NX*NY; constexpr float Cell=40;
        auto Point=[](int32 I){return FVector2D(-1040.f+(I%NX)*Cell,-440.f+(I/NX)*Cell);};
        auto Nearest=[&](FVector2D P){
            int32 Best=-1;float Dist=MAX_flt;
            for(int32 I=0;I<N;++I){const float D=FVector2D::DistSquared(P,Point(I));if(D<Dist && D<160.f*160.f && Segment(P,Point(I))){Best=I;Dist=D;}}
            return Best;
        };
        const int32 First=Nearest(S),Last=Nearest(G);if(First<0||Last<0)return false;
        TArray<float> Cost;Cost.Init(MAX_flt,N);TArray<int32> Parent;Parent.Init(-1,N);
        TArray<uint8> State;State.Init(0,N);TArray<int32> Open;Open.Add(First);Cost[First]=0;State[First]=1;
        while(!Open.IsEmpty())
        {
            int32 Best=0;float Score=MAX_flt;
            for(int32 I=0;I<Open.Num();++I){const int32 V=Open[I];const float F=Cost[V]+FVector2D::Distance(Point(V),Point(Last));if(F<Score){Score=F;Best=I;}}
            const int32 V=Open[Best];Open.RemoveAtSwap(Best);State[V]=2;if(V==Last)break;
            for(int32 DY=-1;DY<=1;++DY)for(int32 DX=-1;DX<=1;++DX)
            {
                if(!DX&&!DY)continue;const int32 X=V%NX+DX,Y=V/NX+DY;if(X<0||X>=NX||Y<0||Y>=NY)continue;
                const int32 U=Y*NX+X;if(State[U]==2||!Segment(Point(V),Point(U)))continue;
                const float C=Cost[V]+FVector2D::Distance(Point(V),Point(U));if(C>=Cost[U])continue;
                Cost[U]=C;Parent[U]=V;if(State[U]==0){Open.Add(U);State[U]=1;}
            }
        }
        if(First!=Last&&Parent[Last]<0)return false;
        TArray<FVector2D> Reverse;for(int32 I=Last;I>=0;I=Parent[I]){Reverse.Add(Point(I));if(I==First)break;}
        TArray<FVector2D> Raw;Raw.Add(S);for(int32 I=Reverse.Num()-1;I>=0;--I)Raw.Add(Reverse[I]);Raw.Add(G);
        for(int32 I=0;I<Raw.Num()-1;)
        {
            int32 J=Raw.Num()-1;while(J>I+1&&!Segment(Raw[I],Raw[J]))--J;
            Out.Add(FVector(Raw[J].X,Raw[J].Y,Height(Raw[J].Y)));I=J;
        }
        return !Out.IsEmpty();
    }
};
