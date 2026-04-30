// ============================================================================
// PROPERTIES OPERATIONS
// ============================================================================

db = db.getSiblingDB("mongo_real_estate_db");

/**
 * API: Добавление объекта недвижимости (Полиморфная вставка)
 * POST /properties
 */
function createProperty(ownerId, type, title, city, price, details) {

    const property = {
        owner_id: ownerId,
        type: type,
        title: title,
        city: city,
        price: price,
        details: details,
        status: "active",
        created_at: new Date(),
        features: []
    };

    const result = db.properties.insertOne(property);

    print("[Mongo] Property created with ID:");
    printjson(result.insertedId);

    return result.insertedId;
}

/**
 * API: Поиск объектов по городу и типу
 * GET /properties?city=Moscow&type=apartment
 */
function findPropertiesByCity(city) {

    return db.properties.find({
        city: city
    }).toArray();
}

/**
 * API: Поиск объектов по цене (используем $gt, $lt)
 * GET /properties?min_price=1000000&max_price=5000000
 */
function findPropertiesByPriceRange(minPrice, maxPrice) {

    return db.properties.find({
        price: {
            $gte: minPrice,
            $lte: maxPrice
        }
    }).toArray();
}

/**
 * API: Добавление удобства в список (Оператор $addToSet)
 * PATCH /properties/{id}/features
 */
function addFeatureToProperty(propertyId, feature) {

    const result = db.properties.updateOne(
        { _id: propertyId },
        { $addToSet: { features: feature } }
    );

    return result.modifiedCount > 0;
}

/**
 * API: Изменение статуса объекта ($set)
 */
function updatePropertyStatus(propertyId, newStatus) {

    db.properties.updateOne(
        { _id: propertyId },
        {
            $set: {
                status: newStatus,
                updated_at: new Date()
            }
        }
    );
}

// ============================================================================
// VIEWINGS OPERATIONS
// ============================================================================

/**
 * API: Запись на просмотр объекта
 * POST /properties/{id}/viewings
 */
function createViewing(propertyId, userId, scheduledTime) {

    const viewing = {
        property_id: propertyId,
        user_id: userId,
        scheduled_time: new Date(scheduledTime),
        status: "pending",
        comments: [],
        created_at: new Date()
    };

    const result = db.viewings.insertOne(viewing);

    return result.insertedId;
}

/**
 * API: Добавление комментария/запроса на перенос времени (Оператор $push)
 * POST /viewings/{id}/comment
 */
function addCommentToViewing(viewingId, text, author) {

    const commentDoc = {
        text: text,
        author: author,
        timestamp: new Date()
    };

    const result = db.viewings.updateOne(
        { _id: viewingId },
        { $push: { comments: commentDoc } }
    );

    return result.modifiedCount > 0;
}

/**
 * API: Перенос времени просмотра (Сложное обновление: $set + $push)
 * PATCH /viewings/{id}/reschedule
 */
function rescheduleViewing(viewingId, newTime, reason) {

    const historyEntry = {
        text: `Rescheduled to ${newTime}. Reason: ${reason}`,
        author: "system",
        timestamp: new Date()
    };

    const result = db.viewings.updateOne(
        { _id: viewingId },
        {
            $set: {
                scheduled_time: new Date(newTime)
            },
            $push: {
                comments: historyEntry
            }
        }
    );

    return result.modifiedCount > 0;
}

/**
 * API: Удаление последнего комментария (Оператор $pop)
 * DELETE /viewings/{id}/last-comment
 */
function removeLastComment(viewingId) {

    const result = db.viewings.updateOne(
        { _id: viewingId },
        { $pop: { comments: 1 } }
    );

    return result.modifiedCount > 0;
}

/**
 * API: Получение записей на просмотр объекта
 */
function getViewingsForProperty(propertyId) {

    return db.viewings
        .find({ property_id: propertyId })
        .sort({ scheduled_time: 1 })
        .toArray();
}