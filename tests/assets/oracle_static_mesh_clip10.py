"""Independent exact rational geometric oracle for the bounded clipping fixture.
No VU instruction execution; rational intersections independently fix the
polygon order, projection and interpolated color used by the C fixture test.
"""
from fractions import Fraction as F
import json
points=[([F(-1,2),F(-1,2),F(0),F(1)],[0,40,80,100]),
        ([F(2),F(-1,2),F(0),F(1)],[100,140,180,200]),
        ([F(-1,2),F(1,2),F(0),F(1)],[200,240,40,0])]
for axis,side in [(2,-1),(2,1),(0,-1),(0,1),(1,-1),(1,1)]:
    result=[]
    for i,(p,pc) in enumerate(points):
        q,qc=points[(i+1)%len(points)]
        dp,dq=p[axis]-side*p[3],q[axis]-side*q[3]
        pin,qin=dp*side<=0,dq*side<=0
        if pin: result.append((p,pc))
        if pin!=qin:
            t=abs(dp/(dq-dp))
            result.append(([a+(b-a)*t for a,b in zip(p,q)],
                           [a+(b-a)*t for a,b in zip(pc,qc)]))
    points=result
print(json.dumps([dict(clip=list(map(str,p)),xy_fixed4=[int((p[j]*100+2048*p[3])/p[3]*16) for j in (0,1)],rgba=list(map(int,c))) for p,c in points],indent=2))
