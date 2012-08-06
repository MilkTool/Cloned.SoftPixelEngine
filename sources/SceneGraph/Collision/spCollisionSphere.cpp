/*
 * Collision sphere file
 * 
 * This file is part of the "SoftPixel Engine" (Copyright (c) 2008 by Lukas Hermanns)
 * See "SoftPixelEngine.hpp" for license information.
 */

#include "SceneGraph/Collision/spCollisionSphere.hpp"
#include "SceneGraph/Collision/spCollisionCapsule.hpp"
#include "SceneGraph/Collision/spCollisionBox.hpp"
#include "SceneGraph/Collision/spCollisionPlane.hpp"
#include "SceneGraph/Collision/spCollisionMesh.hpp"
#include "SceneGraph/Collision/spCollisionConfigTypes.hpp"

#include <boost/foreach.hpp>


namespace sp
{
namespace scene
{


CollisionSphere::CollisionSphere(
    CollisionMaterial* Material, SceneNode* Node, f32 Radius) :
    CollisionNode   (Material, Node, COLLISION_SPHERE   ),
    Radius_         (Radius                             )
{
}
CollisionSphere::~CollisionSphere()
{
}

bool CollisionSphere::checkIntersection(const dim::line3df &Line, SIntersectionContact &Contact) const
{
    const dim::vector3df SpherePos(getPosition());
    
    /* Make an intersection test with the line and this sphere */
    if (math::CollisionLibrary::checkLineSphereIntersection(Line, SpherePos, Radius_, Contact.Point))
    {
        Contact.Normal = (Contact.Point - SpherePos).normalize();
        Contact.Object = this;
        return true;
    }
    
    return false;
}

bool CollisionSphere::checkIntersection(const dim::line3df &Line) const
{
    /* Make an intersection test with the line and this sphere */
    dim::vector3df Tmp;
    return math::CollisionLibrary::checkLineSphereIntersection(Line, getPosition(), Radius_, Tmp);
}


/*
 * ======= Private: =======
 */

bool CollisionSphere::checkCollisionToSphere(const CollisionSphere* Rival, SCollisionContact &Contact) const
{
    if (!Rival)
        return false;
    
    /* Store transformation */
    const dim::vector3df SpherePos(getPosition());
    const dim::vector3df OtherPos(Rival->getPosition());
    
    /* Check if this object and the other collide with each other */
    if (math::getDistanceSq(SpherePos, OtherPos) < math::Pow2(Radius_ + Rival->getRadius()))
    {
        Contact.Normal  = (SpherePos - OtherPos).normalize();
        Contact.Point   = OtherPos + Contact.Normal * Rival->getRadius();
        return true;
    }
    
    return false;
}

bool CollisionSphere::checkCollisionToCapsule(const CollisionCapsule* Rival, SCollisionContact &Contact) const
{
    if (!Rival)
        return false;
    
    /* Store transformation */
    const dim::vector3df SpherePos(getPosition());
    const dim::line3df CapsuleLine(Rival->getLine());
    
    /* Get the closest point from this sphere to the capsule */
    const dim::vector3df ClosestPoint = CapsuleLine.getClosestPoint(SpherePos);
    
    /* Check if this object and the other collide with each other */
    if (math::getDistanceSq(SpherePos, ClosestPoint) < math::Pow2(Radius_ + Rival->getRadius()))
    {
        Contact.Normal  = (SpherePos - ClosestPoint).normalize();
        Contact.Point   = ClosestPoint + Contact.Normal * Rival->getRadius();
        return true;
    }
    
    return false;
}

bool CollisionSphere::checkCollisionToBox(const CollisionBox* Rival, SCollisionContact &Contact) const
{
    if (!Rival)
        return false;
    
    /* Store transformation */
    const dim::matrix4f Mat(Rival->getTransformation().getPositionRotationMatrix());
    const dim::matrix4f InvMat(Mat.getInverse());
    
    const dim::aabbox3df Box(Rival->getBox().getScaled(getScale()));
    const dim::vector3df SpherePos(getPosition());
    const dim::vector3df SphereInvPos(InvMat * SpherePos);
    
    /* Get the closest point from this box and the box */
    const dim::vector3df Point = math::CollisionLibrary::getClosestPoint(Box, SphereInvPos);
    
    /* Check if this object and the other collide with each other */
    if (math::getDistanceSq(Point, SphereInvPos) < math::Pow2(getRadius()))
    {
        Contact.Point   = Mat * Point;
        Contact.Normal  = (SpherePos - Contact.Point).normalize();
        return true;
    }
    
    return false;
}

bool CollisionSphere::checkCollisionToPlane(const CollisionPlane* Rival, SCollisionContact &Contact) const
{
    if (!Rival)
        return false;
    
    /* Store transformation */
    const dim::vector3df SpherePos(getPosition());
    const dim::plane3df RivalPlane(
        Rival->getTransformation().getPositionRotationMatrix() * Rival->getPlane()
    );
    
    /* Check if this object and the other collide with each other */
    if (RivalPlane.getPointDistance(SpherePos) < getRadius())
    {
        Contact.Point   = RivalPlane.getClosestPoint(SpherePos);
        Contact.Normal  = RivalPlane.Normal;
        
        return true;
    }
    
    return false;
}

bool CollisionSphere::checkCollisionToMesh(const CollisionMesh* Rival, SCollisionContact &Contact) const
{
    if (!Rival)
        return false;
    
    /* Check if rival mesh has a tree-hierarchy */
    KDTreeNode* RootTreeNode = Rival->getRootTreeNode();
    
    if (!RootTreeNode)
        return false;
    
    /* Store transformation */
    const dim::vector3df SpherePos(getPosition());
    const video::EFaceTypes CollFace(Rival->getCollFace());
    
    const dim::matrix4f RivalMat(Rival->getTransformation());
    const dim::matrix4f RivalMatInv(RivalMat.getInverse());
    
    const dim::vector3df SpherePosInv(RivalMatInv * SpherePos);
    
    f32 DistanceSq = math::Pow2(getRadius());
    SCollisionFace* ClosestFace = 0;
    dim::vector3df ClosestPoint;
    
    std::map<SCollisionFace*, bool> FaceMap;
    
    /* Get tree node list */
    std::list<const TreeNode*> TreeNodeList;
    
    RootTreeNode->findLeafList(
        TreeNodeList, SpherePosInv, (RivalMatInv.getScale() * getRadius()).getMax()
    );
    
    /* Check collision with triangles of each tree-node */
    foreach (const TreeNode* Node, TreeNodeList)
    {
        /* Get tree node data */
        CollisionMesh::TreeNodeDataType* TreeNodeData = static_cast<CollisionMesh::TreeNodeDataType*>(Node->getUserData());
        
        if (!TreeNodeData)
            continue;
        
        /* Check collision with each triangle */
        foreach (SCollisionFace* Face, *TreeNodeData)
        {
            /* Check for unique usage */
            if (FaceMap.find(Face) != FaceMap.end())
                continue;
            
            FaceMap[Face] = true;
            
            /* Check for face-culling */
            if (Face->isBackFaceCulling(CollFace, SpherePosInv))
                continue;
            
            /* Make sphere-triangle collision test */
            const dim::vector3df CurClosestPoint(
                math::CollisionLibrary::getClosestPoint(RivalMat * Face->Triangle, SpherePos)
            );
            
            /* Check if this is a potentially new closest face */
            const f32 CurDistSq = math::getDistanceSq(CurClosestPoint, SpherePos);
            
            if (CurDistSq < DistanceSq)
            {
                /* Store link to new closest face */
                DistanceSq      = CurDistSq;
                ClosestPoint    = CurClosestPoint;
                ClosestFace     = Face;
            }
        }
    }
    
    /* Check if a collision has been detected */
    if (ClosestFace)
    {
        Contact.Normal  = (RivalMat * ClosestFace->Triangle).getNormal();
        Contact.Point   = ClosestPoint;
        Contact.Face    = ClosestFace;
        return true;
    }
    
    return false;
}

bool CollisionSphere::checkAnyCollisionToMesh(const CollisionMesh* Rival) const
{
    if (!Rival)
        return false;
    
    /* Check if rival mesh has a tree-hierarchy */
    KDTreeNode* RootTreeNode = Rival->getRootTreeNode();
    
    if (!RootTreeNode)
        return false;
    
    /* Store transformation */
    const dim::vector3df SpherePos(getPosition());
    const video::EFaceTypes CollFace(Rival->getCollFace());
    
    const dim::matrix4f RivalMat(Rival->getTransformation());
    const dim::matrix4f RivalMatInv(RivalMat.getInverse());
    
    const dim::vector3df SpherePosInv(RivalMatInv * SpherePos);
    
    const f32 RadiusSq = math::Pow2(getRadius());
    
    std::map<SCollisionFace*, bool> FaceMap;
    
    /* Get tree node list */
    std::list<const TreeNode*> TreeNodeList;
    
    RootTreeNode->findLeafList(
        TreeNodeList, SpherePosInv, (RivalMatInv.getScale() * getRadius()).getMax()
    );
    
    /* Check collision with triangles of each tree-node */
    foreach (const TreeNode* Node, TreeNodeList)
    {
        /* Get tree node data */
        CollisionMesh::TreeNodeDataType* TreeNodeData = static_cast<CollisionMesh::TreeNodeDataType*>(Node->getUserData());
        
        if (!TreeNodeData)
            continue;
        
        /* Check collision with each triangle */
        foreach (SCollisionFace* Face, *TreeNodeData)
        {
            /* Check for unique usage */
            if (FaceMap.find(Face) != FaceMap.end())
                continue;
            
            FaceMap[Face] = true;
            
            /* Check for face-culling */
            if (Face->isBackFaceCulling(CollFace, SpherePosInv))
                continue;
            
            /* Make sphere-triangle collision test */
            const dim::vector3df CurClosestPoint(
                math::CollisionLibrary::getClosestPoint(RivalMat * Face->Triangle, SpherePos)
            );
            
            /* Check if the first collision has been detected and return on succeed */
            if (math::getDistanceSq(CurClosestPoint, SpherePos) < RadiusSq)
                return true;
        }
    }
    
    return false;
}

void CollisionSphere::performCollisionResolvingToSphere(const CollisionSphere* Rival)
{
    SCollisionContact Contact;
    if (checkCollisionToSphere(Rival, Contact))
        performDetectedContact(Rival, Contact);
}

void CollisionSphere::performCollisionResolvingToCapsule(const CollisionCapsule* Rival)
{
    SCollisionContact Contact;
    if (checkCollisionToCapsule(Rival, Contact))
        performDetectedContact(Rival, Contact);
}

void CollisionSphere::performCollisionResolvingToBox(const CollisionBox* Rival)
{
    SCollisionContact Contact;
    if (checkCollisionToBox(Rival, Contact))
        performDetectedContact(Rival, Contact);
}

void CollisionSphere::performCollisionResolvingToPlane(const CollisionPlane* Rival)
{
    SCollisionContact Contact;
    if (checkCollisionToPlane(Rival, Contact))
        performDetectedContact(Rival, Contact);
}

void CollisionSphere::performCollisionResolvingToMesh(const CollisionMesh* Rival)
{
    if (!Rival)
        return;
    
    /* Check if rival mesh has a tree-hierarchy */
    KDTreeNode* RootTreeNode = Rival->getRootTreeNode();
    
    if (!RootTreeNode)
        return;
    
    /* Store transformation */
    const video::EFaceTypes CollFace(Rival->getCollFace());
    
    const dim::matrix4f RivalMat(Rival->getTransformation());
    const dim::matrix4f RivalMatInv(RivalMat.getInverse());
    
    dim::vector3df SpherePos(getPosition());
    dim::vector3df SpherePosInv(RivalMatInv * SpherePos);
    
    dim::vector3df ClosestPoint;
    
    std::map<SCollisionFace*, bool> FaceMap, EdgeFaceMap;
    
    /* Get tree node list */
    std::list<const TreeNode*> TreeNodeList;
    
    RootTreeNode->findLeafList(
        TreeNodeList, SpherePosInv, (RivalMatInv.getScale() * getRadius()).getMax()
    );
    
    /* Check collision with triangles of each tree-node */
    foreach (const TreeNode* Node, TreeNodeList)
    {
        /* Get tree node data */
        CollisionMesh::TreeNodeDataType* TreeNodeData = static_cast<CollisionMesh::TreeNodeDataType*>(Node->getUserData());
        
        if (!TreeNodeData)
            continue;
        
        /* Check collision with each triangle face */
        foreach (SCollisionFace* Face, *TreeNodeData)
        {
            /* Check for unique usage */
            if (FaceMap.find(Face) != FaceMap.end())
                continue;
            
            FaceMap[Face] = true;
            
            /* Check for face-culling */
            if (Face->isBackFaceCulling(CollFace, SpherePosInv))
                continue;
            
            /* Make sphere-triangle collision test */
            const dim::triangle3df Triangle(RivalMat * Face->Triangle);
            
            if (!math::CollisionLibrary::getClosestPointStraight(Triangle, SpherePos, ClosestPoint))
                continue;
            
            /* Check if this is a potentially new closest face */
            if (math::getDistanceSq(ClosestPoint, SpherePos) < math::Pow2(getRadius()))
            {
                /* Perform detected collision contact */
                SCollisionContact Contact;
                {
                    Contact.Point       = ClosestPoint;
                    Contact.Normal      = Triangle.getNormal();
                    Contact.Triangle    = Triangle;
                    Contact.Face        = Face;
                }
                performDetectedContact(Rival, Contact);
                
                /* Update sphere position */
                SpherePos       = getPosition();
                SpherePosInv    = RivalMatInv * SpherePos;
            }
        }
    }
    
    /* Check collision with triangles of each tree-node */
    foreach (const TreeNode* Node, TreeNodeList)
    {
        /* Get tree node data */
        CollisionMesh::TreeNodeDataType* TreeNodeData = static_cast<CollisionMesh::TreeNodeDataType*>(Node->getUserData());
        
        if (!TreeNodeData)
            continue;
        
        /* Check collision with each triangle edge */
        foreach (SCollisionFace* Face, *TreeNodeData)
        {
            /* Check for unique usage */
            if (EdgeFaceMap.find(Face) != EdgeFaceMap.end())
                continue;
            
            EdgeFaceMap[Face] = true;
            
            /* Check for face-culling */
            if (Face->isBackFaceCulling(CollFace, SpherePosInv))
                continue;
            
            /* Make sphere-triangle collision test */
            const dim::triangle3df Triangle(RivalMat * Face->Triangle);
            
            ClosestPoint = math::CollisionLibrary::getClosestPoint(Triangle, SpherePos);
            
            /* Check if this is a potentially new closest face */
            if (math::getDistanceSq(ClosestPoint, SpherePos) < math::Pow2(getRadius()))
            {
                /* Perform detected collision contact */
                SCollisionContact Contact;
                {
                    Contact.Point       = ClosestPoint;
                    Contact.Normal      = (SpherePos - ClosestPoint).normalize();
                    Contact.Triangle    = Triangle;
                    Contact.Face        = Face;
                }
                performDetectedContact(Rival, Contact);
                
                /* Update sphere position */
                SpherePos       = getPosition();
                SpherePosInv    = RivalMatInv * SpherePos;
            }
        }
    }
}

void CollisionSphere::performDetectedContact(const CollisionNode* Rival, const SCollisionContact &Contact)
{
    setPosition(Contact.Point + Contact.Normal * getRadius());
    notifyCollisionContact(Rival, Contact);
}


} // /namespace scene

} // /namespace sp



// ================================================================================
