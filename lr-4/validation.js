db = db.getSiblingDB("mongo_real_estate_db");

print("Starting MongoDB validation setup...");

// ========================================================================
// PROPERTIES COLLECTION
// ========================================================================

const propertiesValidator = {
    $jsonSchema: {
        bsonType: "object",
        required: ["owner_id", "type", "title", "city", "price", "address", "details", "status"],
        properties: {
            _id: {
                bsonType: "string",
                description: "must be a string (UUID)"
            },
            owner_id: {
                bsonType: ["int", "long"],
                description: "must be an integer and is required (Reference to Postgres User)"
            },
            type: {
                bsonType: "string",
                enum: ["apartment", "house", "land", "commercial"],
                description: "must be one of allowed values"
            },
            title: {
                bsonType: "string",
                minLength: 5,
                description: "must be a string with at least 5 characters"
            },
            city: {
                bsonType: "string",
                pattern: "^[a-zA-Zа-яА-Я\.\\s]+$",
                description: "must contain only letters"
            },
            price: {
                bsonType: ["double", "int", "long", "decimal"],
                minimum: 0,
                description: "must be a non-negative number"
            },
            status: {
                bsonType: "string",
                enum: ["active", "sold", "rented", "archived"]
            },
            address: {
                bsonType: "object"
            },
            details: {
                bsonType: "object"
            },
            features: {
                bsonType: "array",
                items: { bsonType: "string" }
            }
        }
    }
};

if (db.getCollectionNames().includes("properties")) {
    db.runCommand({
        collMod: "properties",
        validator: propertiesValidator,
        validationLevel: "strict",
        validationAction: "error"
    });
    print("Validation applied to existing 'properties'");
} else {
    db.createCollection("properties", {
        validator: propertiesValidator,
        validationLevel: "strict",
        validationAction: "error"
    });
    print("Collection 'properties' created with validation");
}

// ========================================================================
// VIEWINGS COLLECTION
// ========================================================================

const viewingsValidator = {
    $jsonSchema: {
        bsonType: "object",
        required: ["property_id", "user_id", "scheduled_time", "comments"],
        properties: {
            _id: {
                bsonType: "string",
                description: "must be a string (UUID)"
            },
            property_id: {
                bsonType: "string",
                description: "must be string (UUID)"
            },
            user_id: {
                bsonType: ["int", "long"],
                description: "must be integer"
            },
            scheduled_time: {
                bsonType: "date",
                description: "must be a date"
            },
            status: {
                bsonType: "string",
                enum: ["pending", "confirmed", "cancelled", "completed"]
            },
            comments: {
                bsonType: "array",
                items: {
                    bsonType: "object",
                    required: ["text", "author", "timestamp"],
                    properties: {
                        text: { bsonType: "string" },
                        author: { bsonType: "string" },
                        timestamp: { bsonType: "date" }
                    }
                }
            }
        }
    }
};

if (db.getCollectionNames().includes("viewings")) {
    db.runCommand({
        collMod: "viewings",
        validator: viewingsValidator,
        validationLevel: "strict",
        validationAction: "error"
    });
    print("Validation applied to existing 'viewings'");
} else {
    db.createCollection("viewings", {
        validator: viewingsValidator,
        validationLevel: "strict",
        validationAction: "error"
    });
    print("Collection 'viewings' created with validation");
}

print("=== MongoDB validation setup completed ===");