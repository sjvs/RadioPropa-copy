#include "radiopropa/Source.h"
#include "radiopropa/Units.h"

#include "gtest/gtest.h"
#include <stdexcept>

namespace radiopropa {

TEST(SourcePosition, simpleTest) {
	Vector3d position(1, 2, 3);
	SourcePosition source(position);
	ParticleState ps;
	source.prepareParticle(ps);
	EXPECT_EQ(position, ps.getPosition());
}

TEST(SourceMultiplePositions, simpleTest) {
	SourceMultiplePositions source;
	source.add(Vector3d(1, 0, 0), 0.25);
	source.add(Vector3d(2, 0, 0), 0.75);
	ParticleState ps;
	int n1 = 0;
	int n2 = 0;
	for (int i = 0; i < 10000; i++) {
		source.prepareParticle(ps);
		if (ps.getPosition().x == 1)
			n1++;
		else if (ps.getPosition().x == 2)
			n2++;
	}
	EXPECT_NEAR(n1, 2500, 2 * sqrt(2500));
	EXPECT_NEAR(n2, 7500, 2 * sqrt(7500));
}

TEST(SourceUniformSphere, simpleTest) {
	Vector3d center(0, 0, 0);
	double radius = 110;
	SourceUniformSphere source(center, radius);
	ParticleState ps;
	source.prepareParticle(ps);
	double distance = ps.getPosition().getDistanceTo(center);
	EXPECT_GE(radius, distance);
}

TEST(SourceUniformBox, simpleTest) {
	Vector3d origin(-7, -2, 0);
	Vector3d size(13, 55, 192);
	SourceUniformBox box(origin, size);
	ParticleState p;
	box.prepareParticle(p);
	Vector3d pos = p.getPosition();
	EXPECT_LE(origin.x, pos.x);
	EXPECT_LE(origin.y, pos.y);
	EXPECT_LE(origin.z, pos.z);
	EXPECT_GE(size.x, pos.x);
	EXPECT_GE(size.y, pos.y);
	EXPECT_GE(size.z, pos.z);
}

TEST(SourceUniformCylinder, simpleTest) {
	Vector3d center(0, 0, 0);
	double radius = 15;
	double height = 2;
	SourceUniformCylinder cylinder(center, height, radius);
	ParticleState ps;
	cylinder.prepareParticle(ps);
	Vector3d pos = ps.getPosition();
	double R2 = pos.x*pos.x+pos.y*pos.y;
	double H = pow(pos.z*pos.z, 0.5);
	EXPECT_GE(radius*radius, R2);
	EXPECT_GE(height/2., H);
}


TEST(SourceDensityGrid, withInRange) {
	// Create a grid with 10^3 cells ranging from (0, 0, 0) to (10, 10, 10)
	Vector3d origin(0, 0, 0);
	int cells = 10;
	double spacing = 1;
	ref_ptr<ScalarGrid> grid = new ScalarGrid(origin, cells, spacing);
	for (int ix = 0; ix < cells; ix++)
		for (int iy = 0; iy < cells; iy++)
			for (int iz = 0; iz < cells; iz++)
				grid->get(ix, iy, iz) = ix * iy * iz;

	SourceDensityGrid source(grid);
	ParticleState p;

	source.prepareParticle(p);
	Vector3d pos = p.getPosition();

	// dialed positions should be within the volume (0, 0, 0) - (10, 10, 10)
	EXPECT_LE(0, pos.x);
	EXPECT_GE(10, pos.x);
	EXPECT_LE(0, pos.y);
	EXPECT_GE(10, pos.y);
	EXPECT_LE(0, pos.z);
	EXPECT_GE(10, pos.z);
}

TEST(SourceDensityGrid, OneAllowedCell) {
	// Create a grid with 2^3 cells ranging from (0, 0, 0) to (4, 4, 4)
	Vector3d origin(0, 0, 0);
	int cells = 2;
	double spacing = 2;
	ref_ptr<ScalarGrid> grid = new ScalarGrid(origin, cells, spacing);

	// set all but one cells to 0
	for (int ix = 0; ix < cells; ix++)
		for (int iy = 0; iy < cells; iy++)
			for (int iz = 0; iz < cells; iz++)
				grid->get(ix, iy, iz) = 0;

	// set the first cell ((0, 0, 0) to (2, 2, 2))
	grid->get(0, 0, 0) = 1;

	SourceDensityGrid source(grid);
	ParticleState p;

	int nFalse = 0;
	Vector3d mean(0, 0, 0);
	for (int i = 0; i < 10000; i++) {
		source.prepareParticle(p);
		Vector3d pos = p.getPosition();
		mean += pos;
		if ((pos.x < 0) or (pos.x > 2) or (pos.y < 0) or (pos.y > 2)
				or (pos.z < 0) or (pos.z > 2))
			nFalse++;
	}

	// only the first bin should get dialed
	EXPECT_EQ(0, nFalse);

	// mean should be close to (1, 1, 1) if random positions are uniform in (0, 0, 0) - (2, 2, 2)
	mean /= 10000;
	EXPECT_NEAR(1, mean.x, 0.2);
	EXPECT_NEAR(1, mean.y, 0.2);
	EXPECT_NEAR(1, mean.z, 0.2);
}

TEST(SourceDensityGrid1D, withInRange) {
	// Create a grid with 10 cells ranging from 0 to 10
	Vector3d origin(0, 0, 0);
	int nCells = 10;
	double spacing = 1.;
	ref_ptr<ScalarGrid> grid = new ScalarGrid(origin, nCells, 1, 1, spacing);

	// set some values
	for (int i = 0; i < 10; i++) {
		grid->get(i, 0, 0) = 2;
	}

	SourceDensityGrid1D source(grid);
	ParticleState p;

	source.prepareParticle(p);
	Vector3d pos = p.getPosition();
	// dialed position should be within the range 0 - 10
	EXPECT_LE(0, pos.x);
	EXPECT_GE(10, pos.x);
}

TEST(SourceDensityGrid1D, OneAllowedCell) {
	// Test if the only allowed cells is repeatedly selected
	Vector3d origin(0, 0, 0);
	int nCells = 10;
	double spacing = 1.;
	ref_ptr<ScalarGrid> grid = new ScalarGrid(origin, nCells, 1, 1, spacing);

	// set some values
	for (int i = 0; i < 10; i++) {
		grid->get(i, 0, 0) = 0;
	}
	grid->get(5, 0, 0) = 1;

	SourceDensityGrid1D source(grid);
	ParticleState p;

	for (int i = 0; i < 100; i++) {
		source.prepareParticle(p);
		// dialed position should be in range 5-6
		Vector3d pos = p.getPosition();
		EXPECT_LE(5, pos.x);
		EXPECT_GE(6, pos.x);
	}
}




TEST(SourceList, simpleTest) {
	// test if source list works with one source
	SourceList sourceList;
	ref_ptr<Source> source = new Source;
	source->add(new SourcePosition(Vector3d(10, 0, 0)));
	sourceList.add(source);

	ref_ptr<Candidate> c = sourceList.getCandidate();

	EXPECT_EQ(Vector3d(10, 0, 0), c->created.getPosition());
	EXPECT_EQ(Vector3d(10, 0, 0), c->previous.getPosition());
	EXPECT_EQ(Vector3d(10, 0, 0), c->current.getPosition());
}

TEST(SourceList, noSource) {
	// test if an error is thrown when source list empty
	SourceList sourceList;
	EXPECT_THROW(sourceList.getCandidate(), std::runtime_error);
}

TEST(SourceList, luminosity) {
	// test if the sources are dialed according to their luminosities
	SourceList sourceList;

	ref_ptr<Source> source1 = new Source;
	source1->add(new SourceFrequency(100));
	sourceList.add(source1, 80);

	ref_ptr<Source> source2 = new Source;
	source2->add(new SourceFrequency(0));
	sourceList.add(source2, 20);

	double meanE = 0;
	for (int i = 0; i < 1000; i++) {
		ref_ptr<Candidate> c = sourceList.getCandidate();
		meanE += c->created.getFrequency();
	}
	meanE /= 1000;
	EXPECT_NEAR(80, meanE, 4); // this test can stochastically fail
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

} // namespace radiopropa
