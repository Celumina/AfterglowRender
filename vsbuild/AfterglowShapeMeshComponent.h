#pragma once
#include "AfterglowRenderableComponent.h"
class AfterglowShapeMeshComponent : public AfterglowRenderableComponent<AfterglowShapeMeshComponent> {
public: 
	enum class Shape : uint16_t {
		NDCRetangle, 
		// TODO: Other preset shapes...
	};

	Shape shape() const noexcept { return _shape; }
	inline void setShape(Shape shape) noexcept { _shape = shape; }

private:
	Shape _shape = Shape::NDCRetangle;

};

INR_CLASS(AfterglowShapeMeshComponent) {
	INR_BASE_CLASSES<AfterglowRenderableComponent<AfterglowShapeMeshComponent>>;
	INR_FUNCS(
		INR_FUNC(shape), 
		INR_FUNC(setShape)
	);
};
