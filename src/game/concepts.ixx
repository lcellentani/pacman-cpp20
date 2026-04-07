module;
#include <concepts>

export module game.concepts;

import engine.input;
import engine.renderer;
import engine.types;

export template<typename T>
concept Drawable = requires(T t, Renderer& r) {
	{ t.draw(r) } -> std::same_as<void>;
};

export template<typename T>
concept Updatable = requires(T t, float dt, const InputState& input) {
	{ t.update(dt, input) } -> std::same_as<void>;
};

export template<typename T>
concept Collidable = requires(T t) {
	{ t.getBounds() } -> std::same_as<AABB>;
};

// Composed concept — the full contract for a game entity.
// Any type entering the variant must satisfy all three.
export template<typename T>
concept GameEntity = Drawable<T> && Updatable<T> && Collidable<T>;