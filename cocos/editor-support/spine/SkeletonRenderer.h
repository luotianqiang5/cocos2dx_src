/******************************************************************************
 * Spine Runtimes Software License
 * Version 2.3
 * 
 * Copyright (c) 2013-2015, Esoteric Software
 * All rights reserved.
 * 
 * You are granted a perpetual, non-exclusive, non-sublicensable and
 * non-transferable license to use, install, execute and perform the Spine
 * Runtimes Software (the "Software") and derivative works solely for personal
 * or internal use. Without the written permission of Esoteric Software (see
 * Section 2 of the Spine Software License Agreement), you may not (a) modify,
 * translate, adapt or otherwise create derivative works, improvements of the
 * Software or develop new applications using the Software or (b) remove,
 * delete, alter or obscure any trademarks or any copyright, trademark, patent
 * or other intellectual property or proprietary rights notices on or in the
 * Software, including any copy thereof. Redistributions in binary or source
 * form must include this license and terms.
 * 
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef SPINE_SKELETONRENDERER_H_
#define SPINE_SKELETONRENDERER_H_

#include <spine/spine.h>
#include "cocos2d.h"

namespace spine {

class PolygonBatch;

/** Draws a skeleton. */
class SkeletonRenderer: public cocos2d::Node, public cocos2d::BlendProtocol {
public:
	static SkeletonRenderer* createWithData (spSkeletonData* skeletonData, bool ownsSkeletonData = false);
	static SkeletonRenderer* createWithFile (const std::string& skeletonDataFile, spAtlas* atlas, float scale = 1);
	static SkeletonRenderer* createWithFile (const std::string& skeletonDataFile, const std::string& atlasFile, float scale = 1);

	virtual void update (float deltaTime) override;
	virtual void draw (cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t transformFlags) override;
	virtual void drawSkeleton (const cocos2d::Mat4& transform, uint32_t transformFlags);
	virtual cocos2d::Rect getBoundingBox () const override;
	virtual void onEnter () override;
	virtual void onExit () override;

	spSkeleton* getSkeleton();

	void setTimeScale(float scale);
	float getTimeScale() const;

	void setDebugSlotsEnabled(bool enabled);
	bool getDebugSlotsEnabled() const;

	void setDebugBonesEnabled(bool enabled);
	bool getDebugBonesEnabled() const;

	// --- Convenience methods for common Skeleton_* functions.
	void updateWorldTransform ();

	void setToSetupPose ();
	void setBonesToSetupPose ();
	void setSlotsToSetupPose ();

	/* Returns 0 if the bone was not found. */
	spBone* findBone (const std::string& boneName) const;
	/* Returns 0 if the slot was not found. */
	spSlot* findSlot (const std::string& slotName) const;
	
	/* Sets the skin used to look up attachments not found in the SkeletonData defaultSkin. Attachments from the new skin are
	 * attached if the corresponding attachment from the old skin was attached. Returns false if the skin was not found.
	 * @param skin May be empty string ("") for no skin.*/
	bool setSkin (const std::string& skinName);
	/** @param skin May be 0 for no skin.*/
	bool setSkin (const char* skinName);
	
	/* Returns 0 if the slot or attachment was not found. */
	spAttachment* getAttachment (const std::string& slotName, const std::string& attachmentName) const;
	/* Returns false if the slot or attachment was not found.
	 * @param attachmentName May be empty string ("") for no attachment. */
	bool setAttachment (const std::string& slotName, const std::string& attachmentName);
	/* @param attachmentName May be 0 for no attachment. */
	bool setAttachment (const std::string& slotName, const char* attachmentName);

    // --- BlendProtocol
    virtual void setBlendFunc (const cocos2d::BlendFunc& blendFunc)override;
    virtual const cocos2d::BlendFunc& getBlendFunc () const override;
    virtual void setOpacityModifyRGB (bool value) override;
    virtual bool isOpacityModifyRGB () const override;

CC_CONSTRUCTOR_ACCESS:
	SkeletonRenderer ();
	SkeletonRenderer (spSkeletonData* skeletonData, bool ownsSkeletonData = false);
	SkeletonRenderer (const std::string& skeletonDataFile, spAtlas* atlas, float scale = 1);
	SkeletonRenderer (const std::string& skeletonDataFile, const std::string& atlasFile, float scale = 1);

	virtual ~SkeletonRenderer ();

	void initWithData (spSkeletonData* skeletonData, bool ownsSkeletonData = false);
	void initWithFile (const std::string& skeletonDataFile, spAtlas* atlas, float scale = 1);
	void initWithFile (const std::string& skeletonDataFile, const std::string& atlasFile, float scale = 1);

	void initialize ();

protected:
	void setSkeletonData (spSkeletonData* skeletonData, bool ownsSkeletonData);
	virtual cocos2d::Texture2D* getTexture (spRegionAttachment* attachment) const;
	virtual cocos2d::Texture2D* getTexture (spMeshAttachment* attachment) const;
	virtual cocos2d::Texture2D* getTexture (spSkinnedMeshAttachment* attachment) const;

	bool _ownsSkeletonData;
	spAtlas* _atlas;
	cocos2d::CustomCommand _drawCommand;
	cocos2d::BlendFunc _blendFunc;
	PolygonBatch* _batch;
	float* _worldVertices;
	bool _premultipliedAlpha;
	spSkeleton* _skeleton;
	float _timeScale;
	bool _debugSlots;
	bool _debugBones;
    /*bugfixed_function_spine==========================begin*/
public:
    spAtlas* getAtlas()
    {
        return _atlas;
    }
    
    /*在Bone上添加精灵===========================*/
    /*定义BoneSprite结构体,保存Sprite和Bone的引用*/
    struct BoneSprite
    {
        spBone*             bone;
        cocos2d::Sprite*    sprite;
        float               uvs[8];
        float               offset[8];
        
        float               orginRotation;
        
        int                 isToSlot;//是否根据slot添加
        
        int                 flipY;
    };
    
    typedef std::map<std::string,BoneSprite>    BoneSpriteMap;
    typedef BoneSpriteMap::iterator             BoneSpriteIter;
    
    /*获取指定Slot上sprite的方法*/
    cocos2d::Sprite* getSpriteForBone(const std::string& boneName);
    void removeBoneSprite(const std::string& boneName);
    void removeAllBoneSprite();
    /*在指定bone上添加sprite的方法*/
    void addSpriteToBone(const std::string& boneName,cocos2d::Sprite* sprite);
    /*在指定slot上添加sprite的方法，使用此方法必须保证原slot上附件图片尺寸大小和精灵纹理大小一致*/
    void addSpriteToSlot(const std::string& slotName,cocos2d::Sprite* sprite,bool filpY);
    
    void setBoneSprite(BoneSprite* slotSprite);
    
    BoneSpriteMap     _boneSprites;
    
    void setSlotFlipY(const std::string& slotName,bool flip);
protected:
    void computeWorldVertices(BoneSprite* boneSprite,float* vertices);
    
    void drawBoneSprite(spSlot* slot);
    
    void setSlotScreenPos(spSlot* slot);
    
    void flipWorldVertices_Y();
    /*===============================================end*/
};

}

#endif /* SPINE_SKELETONRENDERER_H_ */
