// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Part/App/FeaturePartCommon.h"
#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

class FeaturePartCommonTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestFile();
        _common = static_cast<Part::Common*>(_doc->addObject("Part::Common"));
    }

    void TearDown() override
    {}

    Part::Common* _common;
};

TEST_F(FeaturePartCommonTest, testIntersecting)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box2obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    // Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    // If we wanted to be excessive, we could check the bounds:
    // EXPECT_EQ(bb.MinX, 0);
    // EXPECT_EQ(bb.MinY, 1);
    // EXPECT_EQ(bb.MinZ, 0);
    // EXPECT_EQ(bb.MaxX, 1);
    // EXPECT_EQ(bb.MaxY, 2);
    // EXPECT_EQ(bb.MaxZ, 3);
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume, 3.0);
}

TEST_F(FeaturePartCommonTest, testNonIntersecting)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box3obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FALSE(bb.IsValid());
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume, 0.0);
}

TEST_F(FeaturePartCommonTest, testTouching)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box4obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FALSE(bb.IsValid());
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume, 0.0);
}

TEST_F(FeaturePartCommonTest, testAlmostTouching)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box5obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FALSE(bb.IsValid());
    double volume = PartTestHelpers::getVolume(ts.getShape());
    EXPECT_DOUBLE_EQ(volume, 0.0);
}

TEST_F(FeaturePartCommonTest, testBarelyIntersecting)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box6obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_EQ(bb.MinX, 0);
    EXPECT_EQ(bb.MinY, 1.9999);
    EXPECT_EQ(bb.MinZ, 0);
    EXPECT_EQ(bb.MaxX, 1);
    EXPECT_EQ(bb.MaxY, 2);
    EXPECT_EQ(bb.MaxZ, 3);
    double volume = PartTestHelpers::getVolume(ts.getShape());
    double target = Base::Precision::Confusion() * 3 * 1000;
    // FLOAT, not DOUBLE here, because ULP accuracy would be too precise.
    EXPECT_FLOAT_EQ(volume, target);  // 0.00029999999999996696);
}

TEST_F(FeaturePartCommonTest, testMustExecute)
{
    // Act
    short mE = _common->mustExecute();
    // Assert
    EXPECT_FALSE(mE);
    _common->Base.setValue(_box1obj);
    // Assert
    mE = _common->mustExecute();
    EXPECT_FALSE(mE);
    // Act
    _common->Tool.setValue(_box2obj);
    // Assert
    mE = _common->mustExecute();
    EXPECT_TRUE(mE);
    _doc->recompute();
    mE = _common->mustExecute();
    EXPECT_FALSE(mE);
}

TEST_F(FeaturePartCommonTest, testGetProviderName)
{
    // Act
    _common->execute();
    const char* name = _common->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderBoolean");
}

namespace Part
{
void PrintTo(ShapeHistory sh, std::ostream* os)
{
    const char* types[] =
        {"Compound", "CompSolid", "Solid", "Shell", "Face", "Wire", "Edge", "Vertex", "Shape"};
    *os << "History for " << types[sh.type] << " is ";
    for (const auto& it : sh.shapeMap) {
        int old_shape_index = it.first;
        *os << " " << old_shape_index << ": ";
        if (!it.second.empty()) {
            for (auto it2 : it.second) {
                *os << it2 << " ";
            }
        }
    }
    *os << std::endl;
}
}  // namespace Part

TEST_F(FeaturePartCommonTest, testHistory)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box2obj);

    // Act and Assert
    std::vector<Part::ShapeHistory> hist = _common->History.getValues();
    EXPECT_EQ(hist.size(), 0);

    // This creates the histories classically generated by FreeCAD for comparison
    using MapList = std::map<int, std::vector<int>>;
    using List = std::vector<int>;
    MapList compare1 =
        {{0, List {0}}, {1, List {5}}, {2, List()}, {3, List {2}}, {4, List {3}}, {5, List {1}}};
    MapList compare2 =
        {{0, List {0}}, {1, List {5}}, {2, List {4}}, {3, List()}, {4, List {3}}, {5, List {1}}};

    _common->execute();
    hist = _common->History.getValues();
    EXPECT_EQ(hist.size(), 2);
    EXPECT_EQ(hist[0].shapeMap, compare1);
    EXPECT_EQ(hist[1].shapeMap, compare2);
    _common->Base.setValue(_box2obj);
    _common->Tool.setValue(_box1obj);
    _common->execute();
    hist = _common->History.getValues();
    // std::cout << testing::PrintToString(hist[0]) << testing::PrintToString(hist[1]);
    EXPECT_EQ(hist.size(), 2);
    EXPECT_EQ(hist[1].shapeMap, compare1);
    EXPECT_EQ(hist[0].shapeMap, compare2);
}
