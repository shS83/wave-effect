import math
import random

import pygame as pg


class WaterMandalaPart:
	def __init__(self, width, height, held_space):
		self.width = width
		self.height = height
		self.held_space = held_space
		self.length = 1
		self.reset(random_y=True)


	def reset(self, random_y=False):
		self.x = random.uniform(-self.held_space, self.held_space)
		self.length = random.uniform(self.width / 100, self.width / 20)
		if random_y:
			self.y = random.uniform(-self.height / 1.5, self.height / 1.5)
		else:
			self.y = -self.height / 1.5 - self.length
		self.speed = random.uniform(0.06, 12.0)
		self.color = (
			random.randint(200, 250),
			random.randint(240, 255),
			255,
			random.randint(2, 30),
		)

	def flow_points(self, amplitudes, change_factors):
		points = []
		length = max(2, int(self.length))

		for i in range(length):
			x = (
				self.x
				+ amplitudes
				* math.cos(math.radians(self.y + i))
				* math.sin(math.radians(self.y / change_factors[0] + i / change_factors[0]))
				* math.cos(math.radians(i * 2))
				- 50 * math.sin(math.radians(i / change_factors[1]))
			)
			points.append((x, self.y + i))

		return points

	def update(self, dt):
		if self.y >= self.height / 1.5:
			self.reset(random_y=False)
		else:
			self.y += self.speed * dt


class WaterMandalaEffect:
	def __init__(self, size, density=None):
		self.width, self.height = size
		self.trail = pg.Surface(size, pg.SRCALPHA)
		self.frame = 0.0
		self.parts = []
		self.randomize(density=density)

	def randomize(self, density=None):
		self.held_space = random.uniform(self.width / 20, self.width / 10)
		self.density = density or random.randint(200, 400)
		self.amplitudes = random.uniform(self.width / 5, self.width / 3)
		self.change_factors = [
			random.uniform(3, 10),
			random.uniform(3, 10),
			random.uniform(3, 10),
		]
		self.symmetry = random.choice([7, 8, 9, 10, 11, 12])
		self.parts = [
			WaterMandalaPart(self.width, self.height, self.held_space)
			for _ in range(self.density)
		]
		self.trail.fill((20, 70, 80, 255))

	def update(self, dt):
		self.frame += 60 * dt
		for part in self.parts:
			part.update(dt)

	def draw(self, surface):
		fade = pg.Surface((self.width, self.height), pg.SRCALPHA)
		fade.fill((20, 70, 80, 10))
		self.trail.blit(fade, (0, 0))

		line_layer = pg.Surface((self.width, self.height), pg.SRCALPHA)
		center_x = self.width / 2
		center_y = self.height / 2
		base_rotation = -self.frame / 10

		for k in range(self.symmetry):
			rotation = math.radians(base_rotation + (180 / self.symmetry) * k)
			cos_r = math.cos(rotation)
			sin_r = math.sin(rotation)

			for part in self.parts:
				points = part.flow_points(self.amplitudes, self.change_factors)
				rotated_points = [
					(
						center_x + x * cos_r - y * sin_r,
						center_y + x * sin_r + y * cos_r,
					)
					for x, y in points
				]

				if len(rotated_points) >= 2:
					pg.draw.lines(line_layer, part.color, False, rotated_points, 1)

		self.trail.blit(line_layer, (0, 0))
		surface.blit(self.trail, (0, 0))

def main():
	pg.init()
	screen = pg.display.set_mode((600, 600))
	pg.display.set_caption("Water Mandala")
	clock = pg.time.Clock()
	effect = WaterMandalaEffect(screen.get_size())
	running = True

	while running:
		dt = clock.tick(60) / 1000

		for event in pg.event.get():
			if event.type == pg.QUIT:
				running = False
			elif event.type == pg.KEYDOWN and event.key == pg.K_ESCAPE:
				running = False
			elif event.type == pg.MOUSEBUTTONDOWN:
				effect.randomize(density=random.randint(200, 500))

		effect.update(dt)
		effect.draw(screen)
		pg.display.flip()

pg.quit()

if __name__ == "__main__":
	main()