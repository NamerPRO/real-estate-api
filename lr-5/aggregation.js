db = db.getSiblingDB("mongo_real_estate_db");

print("Running aggregation: market statistics by city");

const pipeline = [
    {
        $match: {
            status: "active"
        }
    },

    {
        $group: {
            _id: "$city",

            count: { $sum: 1 },
            avgPrice: { $avg: "$price" },
            minPrice: { $min: "$price" },
            maxPrice: { $max: "$price" },
            types: { $addToSet: "$type" }
        }
    },

    {
        $project: {
            _id: 0,
            city: "$_id",
            totalObjects: "$count",
            averagePrice: { $round: ["$avgPrice", 0] },
            priceRange: {
                min: "$minPrice",
                max: "$maxPrice"
            },
            availableTypes: "$types"
        }
    },

    {
        $sort: {
            totalObjects: -1
        }
    }
];

const results = db.properties.aggregate(pipeline).toArray();

print("=== Market Statistics by City ===");
printjson(results);

print("=== Aggregation finished ===");