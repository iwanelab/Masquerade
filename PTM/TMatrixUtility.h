#ifndef PTMATRIX_TMATRIXUTILITY
#define PTMATRIX_TMATRIXUTILITY
#include "TVector.h"
#include "TMatrix.h"
/** @file TMatrixUtility.h
    行列初期化ユーティリティー．*/

namespace PTM {

inline void getAxisMap2D(int& x, int& y, int axis)
{
    switch(axis)
    {
    case 'x': case 'X': case '0':
        x = 0; y = 1;
        break;
    case 'y': case 'Y': case '1':
        x = 1; y = 0;
        break;
    default:
        x = 0; y = 1;
        break;
    }
}

/// x/y軸を指定して2×2行列を回転行列に初期化
template <class MD, class AD>
void init_rot(TMatrixBase<2, 2, MD>& m,
                const TVectorBase<2, AD>& a,
                char axis)
{
    int x, y;
    getAxisMap2D(x, y, axis);
    m.col(x) = a.unit();
    m.col(y)[0] = -m.col(x)[1];
    m.col(y)[1] =  m.col(x)[0];
}

/// 2×2行列を回転行列に初期化
template <class D>
void init_rot(TMatrixBase<2, 2, D>& m, typename D::element_type th)
{
    typename D::element_type c = cos(th);
    typename D::element_type s = sin(th);
    m[0][0] = c; m[0][1] = -s;
    m[1][0] = s; m[1][1] = c;
}

inline void getAxisMap3D(int& x, int& y, int& z, int axis)
{
    switch(axis){
    case 'x': case 'X': case '0':
        x = 0; y = 1; z = 2;
        break;
    case 'y': case 'Y': case '1':
        x = 1; y = 2; z = 0;
        break;
    case 'z': case 'Z': case '2':
        x = 2; y = 0; z = 1;
        break;
    default:
        x = 0; y = 1; z = 2;
        break;
    }
}

/// axis軸, axis++軸を指定して3×3行列を回転行列に初期化
template <class MD, class AD, class BD>
void init_rot(TMatrixBase<3, 3, MD>& m,
                const TVectorBase<3, AD>& a,
                const TVectorBase<3, BD>& b,
                char axis)
{
    int x, y, z;
    getAxisMap3D(x, y, z, axis);
    m.col(x) = a.unit();
    m.col(y) = (b - (b * m.col(x)) * m.col(x)).unit();
    m.col(z) = m.col(x) % m.col(y);
}

/// 3×3行列をx/y/z軸まわり回転行列に初期化
template <class MD>
void init_rot(TMatrixBase<(3), (3), MD>& m, typename MD::element_type th, char axis)
{
    int x, y, z;
    getAxisMap3D(x, y, z, axis);
    typename MD::element_type c = (typename MD::element_type)cos(th);
    typename MD::element_type s = (typename MD::element_type)sin(th);
    m.item(x,x) = 1; m.item(x, y) = 0; m.item(x, z) = 0;
    m.item(y,x) = 0; m.item(y, y) = c; m.item(y, z) = -s;
    m.item(z,x) = 0; m.item(z, y) = s; m.item(z, z) = c;
}

/** 3×3行列を任意軸まわり回転行列に初期化
        +                                                                      +
        |u^2+(1-u^2)cos(th)      uv(1-cos(th))-wsin(th)  wu(1-cos(th))+vsin(th)|
    R = |uv(1-cos(th))+wsin(th)  v^2+(1-v^2)cos(th)      vw(1-cos(th))-usin(th)|
        |wu(1-cos(th))-vsin(th)  vw(1-cos(th))+usin(th)  w^2+(1-w^2)cos(th)    |
        +                                                                      +*/
template <class MD, class AD>
void init_rot(TMatrixBase<3, 3, MD>& m, typename MD::element_type th,
             const TVectorBase<(3), AD>& axis)
{
    typename MD::element_type s = (typename MD::element_type) sin(th), c = (typename MD::element_type) cos(th);
    typename MD::element_type u = axis[0], v = axis[1], w = axis[2];
    m.item(0,0) = u*u + (1-u*u)*c;
    m.item(1,0) = u*v*(1-c) + w*s;
    m.item(2,0) = w*u*(1-c) - v*s;

    m.item(0,1)  = u*v*(1-c) - w*s;
    m.item(1,1) = v*v + (1-v*v)*c;
    m.item(2,1) = v*w*(1-c) + u*s;
    
    m.item(0,2) = w*u*(1-c) + v*s;
    m.item(1,2) = v*w*(1-c) - u*s;
    m.item(2,2) = w*w + (1-w*w)*c;
}

/** 3×3行列をクォータニオンから任意軸まわり回転行列に初期化    */
template <class MD, class QD>
void init_rot(TMatrixBase<3, 3, MD>& m, const TVectorBase<4, QD>& q)
{
    typedef TMatrixBase<3, 3, MD>::element_type etype;//MAT;
    const int W = 0;
    const int X = 1;
    const int Y = 2;
    const int Z = 3;

    typename etype d = q*q;
    assert(d);
    typename etype s = 2 / d;
    typename etype xs = q[X] * s,   ys = q[Y] * s,   zs = q[Z] * s;
    typename etype wx = q[W] * xs,  wy = q[W] * ys,  wz = q[W] * zs;
    typename etype xx = q[X] * xs,  xy = q[X] * ys,  xz = q[X] * zs;
    typename etype yy = q[Y] * ys,  yz = q[Y] * zs,  zz = q[Z] * zs;

    m.item(0,0) = 1 - (yy + zz);    m.item(0,1) = xy - wz;          m.item(0,2) = xz + wy;
    m.item(1,0) = xy + wz;          m.item(1,1) = 1 - (xx + zz);    m.item(1,2) = yz - wx;
    m.item(2,0) = xz - wy;          m.item(2,1) = yz + wx;          m.item(2,2) = 1 - (xx + yy);
}

/// 2×2行列を単位行列に初期化
template <class D>
void init_unitize(TMatrixBase<2, 2, D>& m)
{
    typename D::element_type z = D::zero(0);
    typename D::element_type u = D::unit(1);
    m.item(0, 0)=u;  m.item(0, 1)=z;
    m.item(1, 0)=z;  m.item(1, 1)=u;
}

/// 3×3行列を単位行列に初期化
template <class D>
void init_unitize(TMatrixBase<3, 3, D>& m)
{
    typename D::element_type z = D::zero(0);
    typename D::element_type u = D::unit(1);
    m.item(0,0)=u;  m.item(0,1)=z;  m.item(0,2)=z;
    m.item(1,0)=z;  m.item(1,1)=u;  m.item(1,2)=z;
    m.item(2,0)=z;  m.item(2,1)=z;  m.item(2,2)=u;
}

/// 4×4行列を単位行列に初期化
template <class D>
void init_unitize(TMatrixBase<4, 4, D>& m)
{
    typedef typename D::zero zero;
    typedef typename D::unit unit;
    typename D::element_type z = zero(0);
    typename D::element_type u = unit(1);
    m.item(0,0)=u;  m.item(0,1)=z;  m.item(0,2)=z;  m.item(0,3)=z;
    m.item(1,0)=z;  m.item(1,1)=u;  m.item(1,2)=z;  m.item(1,3)=z;
    m.item(2,0)=z;  m.item(2,1)=z;  m.item(2,2)=u;  m.item(2,3)=z;
    m.item(3,0)=z;  m.item(3,1)=z;  m.item(3,2)=z;  m.item(3,3)=u;
}

/// N×N行列を単位行列に初期化
template <class M>
void init_unitize(MatrixImp<M>& m)
{
    assert(m.width() == m.height());
    typedef typename M::zero zero;
    typename M::element_type z = zero();
    m.clear(z);
    for (size_t i = 0; i < m.width(); ++i)
        m.item(i,i) = 1;
}

/// 3×3行列をベクトルの外積計算になるように初期化(m*b == v^b).
template <class MD, class D>
void init_cross(TMatrixBase<3, 3, MD>& m, const TVectorBase<3, D>& v)
{
    m.item(0,0) = 0;        m.item(0,1) = -v[2];    m.item(0,2) =  v[1];
    m.item(1,0) = v[2];     m.item(1,1) = 0;        m.item(1,2) = -v[0];
    m.item(2,0) = -v[1];    m.item(2,1) =  v[0];    m.item(2,2) = 0;
}

/// 4×4行列をある点を注視する視点行列に初期化する．
template <class D, class BP>
void init_look_at(TMatrixBase<4, 4, D>& a, const TVectorBase<3, BP>& posi)
{

    typedef TMatrixCol<4, 4, typename D::element_type> TAf;
    typedef TMatrixCol<4, 4, typename D::element_type>::element_type atype;
    typedef TVector<3, typename BP::element_type> TVec;

    typename TVec relv = posi - a.col(3).sub_vector(TSubVectorDim<0, 3>());

    TVector<3, typename D::element_type> tmp = a.col(0).sub_vector(TSubVectorDim<0, 3>());
    typename D::element_type sx = tmp.norm();
    tmp = a.col(1).sub_vector(TSubVectorDim<0,3>());
    typename D::element_type sy = tmp.norm();
    tmp = a.col(2).sub_vector(TSubVectorDim<0,3>());
    typename D::element_type sz = tmp.norm();

    // y -> x
    typename TVec tv;
    tv[0] = relv[0];
    tv[1] = 0;
    tv[2] = relv[2];
    typename TVec ey;
    ey[0] = 0;
    ey[1] = 1;
    ey[2] = 0;
    a.col(0).sub_vector(TSubVectorDim<0, 3>()) = (ey % tv.unit()).unit();
    a.col(2).sub_vector(TSubVectorDim<0, 3>()) = relv.unit();
    typename TVec a1 = a.col(2).sub_vector(TSubVectorDim<0, 3>()) % a.col(0).sub_vector(TSubVectorDim<0, 3>());
    a.col(1).sub_vector(TSubVectorDim<0, 3>()) = a1;

    if (a.item(1, 1) < 0)
    {
        typename TAf rot;
        init_rot(rot.sub_matrix(TSubMatrixDim<0, 0, 3, 3>()), (typename atype)M_PI, 'z');
        a = a * rot;
    }
    a.col(0).sub_vector(TSubVectorDim<0,3>()) *= sx;
    a.col(1).sub_vector(TSubVectorDim<0,3>()) *= sy;
    a.col(2).sub_vector(TSubVectorDim<0,3>()) *= sz;
}

template <class D, class BP, class BT>
void init_look_at(TMatrixBase<4, 4, D>& a,
                  const TVectorBase<3, BP>& posz,
                  const TVectorBase<3, BT>& posy)
{
    typedef typename D::ret_type TAf;
    typedef typename BP::ret_type TVec;
    typename D::element_type sx = a.col(0).sub_vector(TSubVectorDim<0,3>()).norm();
    typename D::element_type sy = a.col(1).sub_vector(TSubVectorDim<0,3>()).norm();
    typename D::element_type sz = a.col(2).sub_vector(TSubVectorDim<0,3>()).norm();

    typename TVec relvz = posz - a.col(3).sub_vector(TSubVectorDim<0, 3>());
    typename TVec relvy = posy - a.col(3).sub_vector(TSubVectorDim<0, 3>());
    
    a.col(2).sub_vector(TSubVectorDim<0, 3>()) = relvz.unit();
    relvy = relvy - (relvy * a.col(2).sub_vector(TSubVectorDim<0, 3>())) * a.col(2).sub_vector(TSubVectorDim<0, 3>());
    a.col(1).sub_vector(TSubVectorDim<0, 3>()) = relvy.unit();
    a.col(0).sub_vector(TSubVectorDim<0, 3>()) = a.col(1).sub_vector(TSubVectorDim<0, 3>()) % a.col(2).sub_vector(TSubVectorDim<0, 3>());
    
    a.col(0).sub_vector(TSubVectorDim<0, 3>()) *= sx;
    a.col(1).sub_vector(TSubVectorDim<0, 3>()) *= sy;
    a.col(2).sub_vector(TSubVectorDim<0, 3>()) *= sz;
}

/// 4×4行列をある点を注視する視点行列に初期化する．
template <class D, class BP>
void init_look_at_gl(TMatrixBase<4, 4, D>& a, const TVectorBase<3, BP>& posi)
{
    typename BP::ret_type posi_ = posi - 2 * (posi - a.col(3).sub_vector(TSubVectorDim<0,3>()));
    init_look_at(a, posi_);
}

template <class D, class BZ, class BY>
void init_look_at_gl(TMatrixBase<4, 4, D>& a,
                     const TVectorBase<3, BZ>& posi,
                     const TVectorBase<3, BY>& posy)
{
    typename BZ::ret_type workAroundForBCB6 = a.col(3).sub_vector(TSubVectorDim<0, 3>());
    typename BZ::ret_type posi_ = posi - 2 * (posi - workAroundForBCB6);
    init_look_at(a, posi_, posy);
}

template <class D, class SD, class ZD>
void init_projection_gl(TMatrixBase<4, 4, D>& a,
                        const TVectorBase<3, SD>& screen_,
                        const TVectorBase<2, ZD>& size_,
                        typename D::element_type front = 1.0f, typename D::element_type back = 10000.0f)
{
    typename SD::ret_type screen(screen_);
    typename ZD::ret_type size(size_);
    if (screen[2] <= 0)
        screen[2] = size[0];

    assert(screen[2] > 0);          //  Check screen's position.
    assert(front > 0);              //  Check front clipping plane.
    assert(back > front);           //  Check back clipping plane.
    
    typedef typename D::element_type ET;
    TVector<2, float> center = screen.sub_vector(TSubVectorDim<0, 2>());
    
    center *= front / screen[2];
    size *= front / screen[2];
    
    typename D::element_type Q = back/(back-front);
    a.item(0,0) = 2*front/size[0];      a.item(1,0) = 0;                    a.item(2,0) = 0;            a.item(3,0) = 0;
    a.item(0,1) = 0;                    a.item(1,1) = 2*front/size[1];      a.item(2,1) = 0;            a.item(3,1) = 0;
    a.item(0,2) = 2*center[0]/size[0];  a.item(1,2) = 2*center[1]/size[1];  a.item(2,2) = -Q;           a.item(3,2) = -1;
    a.item(0,3) = 0;                    a.item(1,3) = 0;                    a.item(2,3) = -Q*front;     a.item(3,3) = 0;
}

template <class D, class SD, class ZD>
void init_projection_d3d(TMatrixBase<4, 4, D>& a,
                         const TVectorBase<3, SD>& screen_,
                         const TVectorBase<2, ZD>& size_,
                         typename D::element_type front = 1.0f, typename D::element_type back = 10000.0f)
{
    typename SD::ret_type screen(screen_);
    typename ZD::ret_type size(size_);
    if (screen[2] <= 0)
        screen[2] = size[0];

    assert(screen[2] > 0);              //  Check screen's position.
    assert(front > 0);                  //  Check front clipping plane.
    assert(back > front);               //  Check back clipping plane.
    
    typedef typename D::element_type ET;
    TSubVector<2, typename TVectorBase<3, SD>::desc > center = screen.sub_vector(TSubVectorDim<0, 2>());
    
    center *= front / screen[2];
    size *= front / screen[2];

    typename D::element_type Q = back/(back-front);
    a.item(0,0) = 2*front/size[0];      a.item(1,0) = 0;                    a.item(2,0) = 0;            a.item(3,0) = 0;
    a.item(0,1) = 0;                    a.item(1,1) = 2*front/size[1];      a.item(2,1) = 0;            a.item(3,1) = 0;
    a.item(0,2) = 2*center[0]/size[0];  a.item(1,2) = 2*center[1]/size[1];  a.item(2,2) = Q;            a.item(3,2) = 1;
    a.item(0,3) = 0;                    a.item(1,3) = 0;                    a.item(2,3) = -Q*front;     a.item(3,3) = 0;
}


/** 4行ベクトルを回転をあらわすクォータニオンとして初期化   */
template <class QD, class T, class AD>
void init_quaternion(TVectorBase<4, QD>& q, T angle, const TVectorBase<3, AD>& axis)
{
    typename QD::element_type d = axis.norm();
    assert(d);
    typename QD::element_type s = (typename QD::element_type)(sin(angle / 2) / d);
    q[0] = TVectorBase<4, QD>::element_type( cos(angle / 2) );
    q.sub_vector(TSubVectorDim<1, 3>()) = s * axis;
}

/** 4行ベクトルを回転をあらわすクォータニオンとして初期化   */
template <class QD, class AD>
void init_quaternion(TVectorBase<4, QD>& q, const TVectorBase<3, AD>& rot)
{
    typedef typename QD::element_type ET;
    typename ET angle = rot.norm();
    typename ET s = (typename ET)(sin(angle / 2));
    q[0] = (typename ET)( cos(angle / 2) );
    if (angle > 1e-10f)
    {
        q.sub_vector(TSubVectorDim<1, 3>()) = s / angle * rot;
    }
    else
    {
        q[1] = (typename ET)0;
        q[2] = (typename ET)0;
        q[3] = (typename ET)0;
    }
}

/** 4行ベクトルを回転をあらわすクォータニオンとして初期化   */
template <class QD, class T>
void init_quaternion(TVectorBase<4, QD>& q, T angle, char axis)
{
    q[0] = (typename QD::element_type) cos(angle / 2);
    int x, y, z;
    getAxisMap3D(x,y,z, axis);
    q[x + 1] = (typename QD::element_type) sin(angle / 2);
    q[y + 1] = 0;
    q[z + 1] = 0;
}

/** 4行ベクトルを回転をあらわすクォータニオンとして初期化   */
template <class QD, class MD>
void init_quaternion(TVectorBase<4, QD>& q, TMatrixBase<3, 3, MD>& m)
{
    // check the diagonal
    double tr = m[0][0] + m[1][1] + m[2][2];
    if (tr > 0)
    {
        double s = sqrt(tr + 1);
        typedef typename QD::element_type QET;
        W() = QET(s / 2);
        s =  QET(0.5 / s);
        X() = QET((m[1][2] - m[2][1]) * s);
        Y() = QET((m[2][0] - m[0][2]) * s);
        Z() = QET((m[0][1] - m[1][0]) * s);
    }
    else
    {  // diagonal is negative
        double q[4];
        int i, j, k;
        int nxt[3] = {1, 2, 0};
        i = 0;
        if (m[1][1] > m[0][0]) i = 1;
        if (m[2][2] > m[i][i]) i = 2;
        j = nxt[i];
        k = nxt[j];
        s = sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.0f);
        q[i] = s * 0.5f;
        if (s)
        {
            s = 0.5 / s;
            q[3] = (m[j][k] - m[k][j]) * s;
            q[j] = (m[i][j] + m[j][i]) * s;
            q[k] = (m[i][k] + m[k][i]) * s;         
            W() = q[0];
            X() = q[1];
            Y() = q[2];
            Z() = q[3];
        }
        else
        {
            W() = 0;
            X() = 1;
            Y() = 0;
            Z() = 0;
        }
    }
}

}   //  namespace PTM
#endif
