number = 0.1
local transform

exports = {
    a = 1
}

function onBegin(entity)
    print("Nothing is true")
    transform = entity:getTransform()
end

function onUpdate(entity, delta)
    transform:setPosition(RTX.Vector3.new(number, 0, -5))
    print(exports.a)
end

function onHit(entity, hit, other)
end

function onEnd(entity)
    print("Everything is permitted")
end