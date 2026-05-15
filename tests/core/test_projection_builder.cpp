#include <gtest/gtest.h>

#include "core/ProjectionBuilder.h"
#include "data_structures/Column.h"

TEST(ProjectionBuilderTest, BuildsReadSchemeAndReturnsLocalIndexes) {
    Scheme source;
    for (size_t i = 0; i < hits::kHitsColumnCount; ++i) {
        source.Add({std::string(hits::HitsColumnNames[i]), "string"});
    }

    ProjectionBuilder projection(source);

    const size_t url = projection.Require(hits::HitsColumn::URL);
    const size_t phrase = projection.Require(hits::HitsColumn::SearchPhrase);
    const size_t url_again = projection.Require(hits::HitsColumn::URL);

    EXPECT_EQ(url, 0U);
    EXPECT_EQ(phrase, 1U);
    EXPECT_EQ(url_again, url);
    EXPECT_EQ(projection.ReadScheme().GetSchemeNames(),
              (std::vector<std::string>{"URL", "SearchPhrase"}));
    EXPECT_EQ(projection.ReadScheme().GetSchemeTypes(),
              (std::vector<ColumnTypes>{ColumnTypes::String,
                                        ColumnTypes::String}));
}

TEST(ProjectionBuilderTest, PreservesSourceColumnTypes) {
    Scheme source;
    for (size_t i = 0; i < hits::kHitsColumnCount; ++i) {
        const std::string type =
            i == hits::Col(hits::HitsColumn::AdvEngineID) ? "int16"
                                                          : "string";
        source.Add({std::string(hits::HitsColumnNames[i]), type});
    }

    ProjectionBuilder projection(source);
    const size_t adv = projection.Require(hits::HitsColumn::AdvEngineID);

    EXPECT_EQ(adv, 0U);
    ASSERT_EQ(projection.ReadScheme().GetSchemeTypes().size(), 1U);
    EXPECT_EQ(projection.ReadScheme().GetSchemeTypes()[adv],
              ColumnTypes::Int16);
}
