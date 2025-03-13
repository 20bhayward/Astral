#include "astral/physics/CellProcessor.h"
#include "astral/physics/Material.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace astral {

CellProcessor::CellProcessor(MaterialRegistry* registry)
    : materialRegistry(registry)
{
    // Initialize random number generator with current time
    random.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
}

void CellProcessor::initializeCellFromMaterial(Cell& cell, MaterialID materialID) const
{
    // Safety check - verify materialID is valid
    // If materilaID is outside valid range, reset to air
    if (materialID < 0 || materialID > 100) { // Assuming we won't have more than 100 material types
        materialID = materialRegistry->getDefaultMaterialID(); // Reset to air if invalid
    }

    // Reset the cell to defaults
    cell.material = materialID;
    cell.temperature = 20.0f; // Standard room temperature
    cell.velocity = glm::vec2(0.0f, 0.0f);
    cell.pressure = 0.0f;
    cell.health = 1.0f;
    cell.lifetime = 0;
    cell.energy = 0.0f;
    cell.charge = 0.0f;
    cell.stateFlags = 0;
    cell.updated = false;
    cell.metadata = 0;
    
    // Apply material-specific properties
    const MaterialProperties& props = materialRegistry->getMaterial(materialID);
    applyMaterialProperties(cell, props);
}

void CellProcessor::applyMaterialProperties(Cell& cell, const MaterialProperties& props) const
{
    // Apply material type-specific properties
    switch (props.type) {
        case MaterialType::EMPTY:
            // Air or void - Nothing special to initialize
            break;
            
        case MaterialType::SOLID:
            // Solid materials have high health, no velocity
            cell.health = 1.0f;
            break;
            
        case MaterialType::POWDER:
            // Powders have variable density and can fall
            cell.health = props.density / 3000.0f; // Normalize density to reasonable health value
            break;
            
        case MaterialType::LIQUID:
            // Liquids have pressure and flow properties
            cell.health = props.density / 2000.0f;
            break;
            
        case MaterialType::GAS:
            // Gases have lifetime, low density, and dispersion
            cell.lifetime = static_cast<uint8_t>(props.lifetime > 0 ? props.lifetime : 255);
            cell.health = props.density / 500.0f; // Gases have lower health/integrity
            break;
            
        case MaterialType::FIRE:
            // Fire is hot, emissive, and has a limited lifetime
            
            // Different properties for different types of fire
            if (props.name == "OilFire") {
                // Oil fire burns hotter and longer
                cell.temperature = 650.0f; // Higher temperature for oil fire
                cell.lifetime = static_cast<uint8_t>(props.lifetime > 0 ? props.lifetime : 150);
                cell.energy = 150.0f; // Oil fire has more energy
            } else {
                // Regular fire
                cell.temperature = 500.0f; // High starting temperature
                cell.lifetime = static_cast<uint8_t>(props.lifetime > 0 ? props.lifetime : 100);
                cell.energy = 100.0f; // Fire has high energy
            }
            
            cell.setFlag(Cell::FLAG_BURNING);
            break;
            
        case MaterialType::SPECIAL:
            // Special materials can have custom behavior
            // Use metadata for special properties
            cell.metadata = 1; // Indicate it has special properties
            break;
    }
    
    // Apply flammability
    if (props.flammable) {
        cell.energy = props.flammability * 10.0f; // Store flammability as potential energy
    }
    
    // Apply conductivity
    if (props.hasFlag(MaterialProperties::Flags::CONDUCTIVE)) {
        cell.charge = 10.0f; // Default conductivity for conductive materials
    }
    
    // Apply thermal properties - set initial temperature based on material
    if (props.type == MaterialType::FIRE) {
        cell.temperature = 600.0f;  // Fire is hot!
    } else if (props.type == MaterialType::GAS && props.name.find("Steam") != std::string::npos) {
        cell.temperature = 120.0f;  // Steam is hot
    } else if (props.type == MaterialType::GAS && props.name.find("Smoke") != std::string::npos) {
        cell.temperature = 150.0f;  // Smoke is hot
    } else if (props.type == MaterialType::LIQUID && props.name.find("Lava") != std::string::npos) {
        cell.temperature = 1000.0f; // Lava is very hot!
    } else if (props.meltingPoint > 0 && props.meltingPoint < 100) {
        // Cold materials (like ice)
        cell.temperature = 0.0f;
    } else if (props.boilingPoint > 0 && props.boilingPoint < 150) {
        // Materials that boil easily (like volatile liquids)
        cell.temperature = props.boilingPoint * 0.5f;
    } else {
        // Standard materials at room temperature
        cell.temperature = 20.0f;
    }
}

bool CellProcessor::canCellMove(const Cell& cell, const Cell& target) const
{
    // Cannot move if the cell is not movable
    const MaterialProperties& cellProps = materialRegistry->getMaterial(cell.material);
    if (!cellProps.movable) {
        return false;
    }
    
    // Can always move into empty space
    if (target.material == materialRegistry->getDefaultMaterialID()) {
        return true;
    }
    
    // Check if densities allow displacement
    const MaterialProperties& targetProps = materialRegistry->getMaterial(target.material);
    
    // Allow any cell to move into air spaces
    if (targetProps.type == MaterialType::EMPTY) {
        return true; 
    }
    
    // Special case for lava - it should behave more consistently
    if (cellProps.name == "Lava") {
        // Lava can always displace water
        if (targetProps.name == "Water") {
            return true;
        }
        
        // Lava can always displace oil (ignites it)
        if (targetProps.name == "Oil") {
            return true;
        }
        
        // Lava can displace most other materials based on density, but not stone or other lava
        if (targetProps.name != "Stone" && targetProps.name != "Lava" && 
            cellProps.density > targetProps.density) {
            return true;
        }
    }
    
    // Sand should be able to fall through lava for realism
    if (cellProps.name == "Sand" && targetProps.name == "Lava") {
        return true;
    }
    
    // Liquids and powders can displace less dense materials, but NOT wood
    if ((cellProps.type == MaterialType::LIQUID || cellProps.type == MaterialType::POWDER) &&
        cellProps.density > targetProps.density) {
        // Special protection for wood - sand/powder shouldn't destroy wood structures
        if (targetProps.type == MaterialType::WOOD || targetProps.name == "Wood") {
            return false;
        }
        return true;
    }
    
    // Gases can displace other gases if they're denser
    if (cellProps.type == MaterialType::GAS && targetProps.type == MaterialType::GAS &&
        cellProps.density > targetProps.density) {
        return true;
    }
    
    return false;
}

bool CellProcessor::canDisplace(const Cell& mover, const Cell& target) const
{
    // Check basic movement capability
    if (!canCellMove(mover, target)) {
        return false;
    }
    
    // Get material properties
    const MaterialProperties& moverProps = materialRegistry->getMaterial(mover.material);
    const MaterialProperties& targetProps = materialRegistry->getMaterial(target.material);
    
    // Empty space is always displaceable
    if (target.material == materialRegistry->getDefaultMaterialID()) {
        return true;
    }
    
    // Calculate density difference
    float densityRatio = moverProps.density / targetProps.density;
    
    // Higher density materials can displace lower density ones
    if (densityRatio > 1.0f) {
        // The greater the density difference, the more likely to displace
        return true;
    }
    
    // Special case for fire spreading to flammable materials
    if (moverProps.type == MaterialType::FIRE && targetProps.flammable) {
        // Fire can "displace" flammable materials by igniting them
        return true;
    }
    
    return false;
}

bool CellProcessor::shouldSwapCells(const Cell& cell1, const Cell& cell2) const
{
    // Identical materials don't need swapping except for pressure equalization
    if (cell1.material == cell2.material) {
        // For liquids and gases, may swap based on pressure
        const MaterialProperties& props = materialRegistry->getMaterial(cell1.material);
        if ((props.type == MaterialType::LIQUID || props.type == MaterialType::GAS) &&
            std::abs(cell1.pressure - cell2.pressure) > 0.1f) {
            return true;
        }
        return false;
    }
    
    // Check if either can displace the other
    if (canDisplace(cell1, cell2)) {
        return true;
    }
    if (canDisplace(cell2, cell1)) {
        return true;
    }
    
    return false;
}

bool CellProcessor::canReact(const Cell& cell1, const Cell& cell2) const
{
    // Get material properties
    const MaterialProperties& props1 = materialRegistry->getMaterial(cell1.material);
    const MaterialProperties& props2 = materialRegistry->getMaterial(cell2.material);
    
    // Check for direct reaction rules in material properties
    for (const auto& reaction : props1.reactions) {
        if (reaction.reactantMaterial == cell2.material) {
            return true;
        }
    }
    
    // Special material type interactions
    
    // Fire can react with flammable materials
    if ((props1.type == MaterialType::FIRE && props2.flammable) ||
        (props2.type == MaterialType::FIRE && props1.flammable)) {
        return true;
    }
    
    // Water can extinguish fire
    if ((props1.type == MaterialType::FIRE && props2.type == MaterialType::LIQUID && 
         props2.name.find("Water") != std::string::npos) ||
        (props2.type == MaterialType::FIRE && props1.type == MaterialType::LIQUID && 
         props1.name.find("Water") != std::string::npos)) {
        return true;
    }
    
    // Acids can dissolve most solid materials
    if ((props1.name.find("Acid") != std::string::npos && props2.type == MaterialType::SOLID) ||
        (props2.name.find("Acid") != std::string::npos && props1.type == MaterialType::SOLID)) {
        return true;
    }
    
    return false;
}

bool CellProcessor::processPotentialReaction(Cell& cell1, Cell& cell2, float deltaTime)
{
    // Check if reaction is possible
    if (!canReact(cell1, cell2)) {
        return false;
    }
    
    // Get material properties
    const MaterialProperties& props1 = materialRegistry->getMaterial(cell1.material);
    const MaterialProperties& props2 = materialRegistry->getMaterial(cell2.material);
    
    // Special case: Lava interactions with other materials
    if (props1.name == "Lava" && cell2.material != materialRegistry->getLavaID()) {
        // Special case for water - creates stone and steam
        if (cell2.material == materialRegistry->getWaterID()) {
            // When lava touches water, it rapidly cools to stone
            if (rollProbability(0.8f)) {
                // Convert lava to stone
                cell1.material = materialRegistry->getStoneID();
                cell1.temperature = 200.0f;  // Still hot but cooled down
                
                // Convert water to steam with high probability
                if (rollProbability(0.85f)) {
                    cell2.material = materialRegistry->getSteamID();
                    cell2.temperature = 150.0f;
                    cell2.lifetime = static_cast<uint8_t>(60 + getRandomInt(0, 20));
                    cell2.velocity.y = -1.0f; // Steam rises upward (negative y)
                }
                return true;
            }
        }
        // Special case for oil - INSTANT ignition from lava, guaranteed
        else if (cell2.material == materialRegistry->getOilID()) {
            // Instantly convert oil to oil fire with no probability check
            cell2.material = materialRegistry->getOilFireID();
            cell2.temperature = 700.0f;  // Extra hot
            cell2.setFlag(Cell::FLAG_BURNING);
            cell2.lifetime = static_cast<uint8_t>(120);  // Long-lasting oil fire from lava
            cell2.energy = 150.0f;       // High energy
            return true;
        }
        // Handle sand - lava should melt sand to more lava
        else if (cell2.material == materialRegistry->getSandID()) {
            // Sand melts into lava when touching lava
            if (rollProbability(0.6f * deltaTime * 10.0f)) {
                cell2.material = materialRegistry->getLavaID();
                cell2.temperature = 1000.0f;
                return true;
            }
        }
        // Normal handling for other flammable materials
        else if (props2.flammable) {
            // Lava ignites flammable materials with high probability
            if (rollProbability(0.7f * deltaTime * 10.0f)) {  // Higher probability for more predictable behavior
                // Convert to fire based on material type
                if (props2.name == "Oil") {
                    cell2.material = materialRegistry->getOilFireID();
                    cell2.temperature = 650.0f;
                } else {
                    cell2.material = materialRegistry->getFireID();
                    cell2.temperature = 550.0f;
                }
                cell2.setFlag(Cell::FLAG_BURNING);
                cell2.lifetime = static_cast<uint8_t>(props2.burnRate * 200.0f);
                return true;
            }
        } 
        // Lava damages and eventually melts non-flammable materials (except stone)
        else if (props2.name != "Stone" && props2.name != "Lava" && props2.name != "Air") {
            // More aggressive damage rate for predictable gameplay
            if (rollProbability(0.4f * deltaTime * 10.0f)) {
                cell2.health -= 0.2f;  // Double the damage rate
                cell2.temperature += 50.0f;  // Heat up the material
                
                if (cell2.health <= 0.0f) {
                    // Higher chance of creating lava instead of just destruction
                    if (rollProbability(0.6f)) {
                        // Convert destroyed material to lava
                        cell2.material = materialRegistry->getLavaID();
                        cell2.temperature = 1000.0f;
                    } 
                    // Sometimes create smoke for effect
                    else if (rollProbability(0.5f)) {
                        cell2.material = materialRegistry->getSmokeID();
                        cell2.temperature = 200.0f;
                        cell2.lifetime = 60;
                    } 
                    // Otherwise just replace with air
                    else {
                        cell2.material = materialRegistry->getDefaultMaterialID();
                    }
                    return true;
                }
            }
        }
    }
    // Also check the reverse direction
    else if (props2.name == "Lava" && cell1.material != materialRegistry->getLavaID()) {
        // Special case for water - creates stone and steam
        if (cell1.material == materialRegistry->getWaterID()) {
            // When lava touches water, it rapidly cools to stone
            if (rollProbability(0.8f)) {
                // Convert lava to stone
                cell2.material = materialRegistry->getStoneID();
                cell2.temperature = 200.0f;  // Still hot but cooled down
                
                // Convert water to steam with high probability
                if (rollProbability(0.85f)) {
                    cell1.material = materialRegistry->getSteamID();
                    cell1.temperature = 150.0f;
                    cell1.lifetime = static_cast<uint8_t>(60 + getRandomInt(0, 20));
                    cell1.velocity.y = -1.0f; // Steam rises upward (negative y)
                }
                return true;
            }
        }
        // Special case for oil - INSTANT ignition from lava, guaranteed
        else if (cell1.material == materialRegistry->getOilID()) {
            // Instantly convert oil to oil fire with no probability check
            cell1.material = materialRegistry->getOilFireID();
            cell1.temperature = 700.0f;  // Extra hot
            cell1.setFlag(Cell::FLAG_BURNING);
            cell1.lifetime = static_cast<uint8_t>(120);  // Long-lasting oil fire from lava
            cell1.energy = 150.0f;       // High energy
            return true;
        }
        // Handle sand - lava should melt sand to more lava
        else if (cell1.material == materialRegistry->getSandID()) {
            // Sand melts into lava when touching lava
            if (rollProbability(0.6f * deltaTime * 10.0f)) {
                cell1.material = materialRegistry->getLavaID();
                cell1.temperature = 1000.0f;
                return true;
            }
        }
        // Normal handling for other flammable materials
        else if (props1.flammable) {
            // Lava ignites flammable materials with high probability
            if (rollProbability(0.7f * deltaTime * 10.0f)) {  // Higher probability for more predictable behavior
                // Convert to fire based on material type
                if (props1.name == "Oil") {
                    cell1.material = materialRegistry->getOilFireID();
                    cell1.temperature = 650.0f;
                } else {
                    cell1.material = materialRegistry->getFireID();
                    cell1.temperature = 550.0f;
                }
                cell1.setFlag(Cell::FLAG_BURNING);
                cell1.lifetime = static_cast<uint8_t>(props1.burnRate * 200.0f);
                return true;
            }
        } 
        // Lava damages and eventually melts non-flammable materials (except stone)
        else if (props1.name != "Stone" && props1.name != "Lava" && props1.name != "Air") {
            // More aggressive damage rate for predictable gameplay
            if (rollProbability(0.4f * deltaTime * 10.0f)) {
                cell1.health -= 0.2f;  // Double the damage rate
                cell1.temperature += 50.0f;  // Heat up the material
                
                if (cell1.health <= 0.0f) {
                    // Higher chance of creating lava instead of just destruction
                    if (rollProbability(0.6f)) {
                        // Convert destroyed material to lava
                        cell1.material = materialRegistry->getLavaID();
                        cell1.temperature = 1000.0f;
                    } 
                    // Sometimes create smoke for effect
                    else if (rollProbability(0.5f)) {
                        cell1.material = materialRegistry->getSmokeID();
                        cell1.temperature = 200.0f;
                        cell1.lifetime = 60;
                    } 
                    // Otherwise just replace with air
                    else {
                        cell1.material = materialRegistry->getDefaultMaterialID();
                    }
                    return true;
                }
            }
        }
    }
    // Check for direct reaction rules in material properties
    for (const auto& reaction : props1.reactions) {
        if (reaction.reactantMaterial == cell2.material) {
            // Apply probability check
            if (rollProbability(reaction.probability * deltaTime * 10.0f)) {
                // Store original target material for byproduct handling
                MaterialID originalMaterial = cell2.material;
                
                // Perform the reaction on the source cell
                cell1.material = reaction.resultMaterial;
                
                // Increase temperature based on the reaction type
                cell1.temperature += 50.0f; // Default heat increase for reactions
                
                // Handle the byproduct if specified
                if (reaction.byproduct != 0) {
                    // Apply byproduct to the target cell
                    cell2.material = reaction.byproduct;
                    
                    // Set appropriate temperatures based on materials
                    const MaterialProperties& byproductProps = materialRegistry->getMaterial(reaction.byproduct);
                    
                    // Lava/water reaction special case
                    if (props1.name.find("Water") != std::string::npos && 
                        byproductProps.category == MaterialCategory::STONE) {
                        // Steam is hot
                        cell1.temperature = 150.0f;
                        // Stone is warm but cooling
                        cell2.temperature = 200.0f;
                    }
                    else if (props1.name.find("Lava") != std::string::npos && 
                             byproductProps.category == MaterialCategory::STONE) {
                        // Steam is hot
                        cell2.temperature = 150.0f;
                        // Stone is warm but cooling
                        cell1.temperature = 200.0f;
                    }
                }
                
                // Set appropriate flag based on material properties
                const MaterialProperties& resultProps = materialRegistry->getMaterial(reaction.resultMaterial);
                if (resultProps.hasFlag(MaterialProperties::Flags::HOT)) {
                    cell1.setFlag(Cell::FLAG_BURNING);
                }
                
                return true;
            }
        }
    }
    
    // Special case: Fire and flammable materials
    if (props1.type == MaterialType::FIRE && props2.flammable) {
        // Check which type of fire (regular or oil fire)
        bool isOilFire = (cell1.material == materialRegistry->getOilFireID());
        float ignitionMultiplier = isOilFire ? 8.0f : 5.0f;  // Oil fire ignites materials more aggressively
        
        // Special treatment for wood - it should burn in place rather than immediately turning to fire
        if (cell2.material == materialRegistry->getWoodID()) {
            // Wood should burn, but slowly enough to spread properly
            
            // If this wood is already burning, check if it will spread to other wood neighbors first
            if (cell2.hasFlag(Cell::FLAG_BURNING)) {
                // Burning wood has a high chance to spread to adjacent wood blocks
                // This is critical for proper fire propagation through wooden structures
                if (rollProbability(0.4f * deltaTime * 10.0f)) {
                    // Try to find adjacent wood cells to spread to
                    // Note: We're using a dummy implementation here because the actual implementation 
                    // would require access to the world grid to find true adjacent cells
                    
                    // The implementation is handled by the fire spreading through high heat transfer
                    // to adjacent wood cells in the transferHeat function
                    // We'll make burning wood hot enough to ignite other wood
                    cell2.temperature = std::max(cell2.temperature, 320.0f);
                }
            }
            
            // Basic burning process - wood should burn VERY slowly
            // Real wood takes minutes to hours to burn - we need to simulate this in accelerated time
            if (rollProbability(0.07f * deltaTime * 10.0f)) {  // Drastically reduced probability for much slower burn
                // Reduce wood health as it burns, at an extremely slow rate
                cell2.health -= 0.003f;  // Extremely slow burn rate
                
                // Set the burning flag on the wood cell
                cell2.setFlag(Cell::FLAG_BURNING);
                
                // Make the wood hot - even hotter to ensure proper fire spread
                cell2.temperature = std::max(cell2.temperature, 400.0f);
                
                // Burning wood has a much higher chance to generate fire and smoke
                // Make sure there's always visible fire coming from burning wood
                if (rollProbability(0.6f)) {  // Increased from 0.2f to 0.6f
                    // Generate smoke and flames around the wood as it burns
                    
                    // 80% chance of creating fire vs 20% smoke - prioritize visible flames
                    if (rollProbability(0.8f)) {
                        // Try to add some fire effects to empty cells
                        Cell* neighborCell = getAdjacentCell(cell2, 0, -1); // Try above first
                        if (neighborCell && neighborCell->material == materialRegistry->getDefaultMaterialID()) {
                            neighborCell->material = materialRegistry->getFireID();
                            neighborCell->temperature = 500.0f;  // Hotter fire
                            neighborCell->setFlag(Cell::FLAG_BURNING);
                            neighborCell->lifetime = static_cast<uint8_t>(50 + getRandomInt(0, 20)); // Longer lasting fire
                            
                            // To make the fire more visibly active, give it an upward velocity
                            neighborCell->velocity.y = -0.5f; // Upward movement
                        }
                    } else {
                        // Try to add some smoke effects to empty cells 
                        Cell* neighborCell = getAdjacentCell(cell2, 0, -1); // Try above first
                        if (neighborCell && neighborCell->material == materialRegistry->getDefaultMaterialID()) {
                            neighborCell->material = materialRegistry->getSmokeID();
                            neighborCell->temperature = 150.0f;  // Hotter smoke
                            neighborCell->lifetime = static_cast<uint8_t>(70 + getRandomInt(0, 20)); // Longer lasting smoke
                            
                            // To make the smoke rise more visibly
                            neighborCell->velocity.y = -0.3f; // Upward movement
                        }
                    }
                }
                
                // When wood is completely burned, it turns to fire and then quickly to ash
                if (cell2.health <= 0.0f) {
                    cell2.material = materialRegistry->getFireID();
                    cell2.temperature = 400.0f;
                    cell2.lifetime = static_cast<uint8_t>(25 + getRandomInt(-5, 5));
                }
                
                return true;
            }
        } 
        // Regular ignition for other materials
        else if (rollProbability(props2.flammability * deltaTime * ignitionMultiplier)) {
            // Oil ignites INSTANTLY and always (100% chance) when in contact with fire or lava
            if (cell2.material == materialRegistry->getOilID()) {
                cell2.material = materialRegistry->getOilFireID();
                cell2.temperature = std::max(cell2.temperature, 650.0f);
                // Make oil fire spread to nearby oil more aggressively
                cell2.temperature += 50.0f;  // Extra hot to ignite nearby oil
                cell2.energy += 50.0f;       // Extra energy for stronger fire
            } else {
                cell2.material = materialRegistry->getFireID();
                cell2.temperature = std::max(cell2.temperature, 500.0f);
            }
            
            cell2.setFlag(Cell::FLAG_BURNING);
            
            // Adjust lifetime based on material type
            float lifetimeScale = 1.0f;
            if (cell2.material == materialRegistry->getOilID()) {
                lifetimeScale = 2.0f;  // Oil burns longer
            }
            
            cell2.lifetime = static_cast<uint8_t>(props2.burnRate * 200.0f * lifetimeScale);
            
            // Generate smoke occasionally during ignition
            if (rollProbability(0.1f)) {
                cell2.temperature += 20.0f;  // Extra heat from combustion
            }
            
            return true;
        }
    }
    else if (props2.type == MaterialType::FIRE && props1.flammable) {
        // Check which type of fire (regular or oil fire)
        bool isOilFire = (cell2.material == materialRegistry->getOilFireID());
        float ignitionMultiplier = isOilFire ? 8.0f : 5.0f;  // Oil fire ignites materials more aggressively
        
        // Special treatment for wood - it should burn in place rather than immediately turning to fire
        if (cell1.material == materialRegistry->getWoodID()) {
            // Wood should burn, but slowly enough to spread properly
            
            // If this wood is already burning, check if it will spread to other wood neighbors first
            if (cell1.hasFlag(Cell::FLAG_BURNING)) {
                // Burning wood has a high chance to spread to adjacent wood blocks
                // This is critical for proper fire propagation through wooden structures
                if (rollProbability(0.4f * deltaTime * 10.0f)) {
                    // Try to find adjacent wood cells to spread to
                    // Note: We're using a dummy implementation here because the actual implementation 
                    // would require access to the world grid to find true adjacent cells
                    
                    // The implementation is handled by the fire spreading through high heat transfer
                    // to adjacent wood cells in the transferHeat function
                    // We'll make burning wood hot enough to ignite other wood
                    cell1.temperature = std::max(cell1.temperature, 320.0f);
                }
            }
            
            // Basic burning process - wood should burn VERY slowly
            // Real wood takes minutes to hours to burn - we need to simulate this in accelerated time
            if (rollProbability(0.07f * deltaTime * 10.0f)) {  // Drastically reduced probability for much slower burn
                // Reduce wood health as it burns, at an extremely slow rate
                cell1.health -= 0.003f;  // Extremely slow burn rate
                
                // Set the burning flag on the wood cell
                cell1.setFlag(Cell::FLAG_BURNING);
                
                // Make the wood hot - even hotter to ensure proper fire spread
                cell1.temperature = std::max(cell1.temperature, 400.0f);
                
                // Burning wood has a much higher chance to generate fire and smoke
                // Make sure there's always visible fire coming from burning wood
                if (rollProbability(0.6f)) {  // Increased from 0.2f to 0.6f
                    // Generate smoke and flames around the wood as it burns
                    
                    // 80% chance of creating fire vs 20% smoke - prioritize visible flames
                    if (rollProbability(0.8f)) {
                        // Try to add some fire effects to empty cells
                        Cell* neighborCell = getAdjacentCell(cell1, 0, -1); // Try above first
                        if (neighborCell && neighborCell->material == materialRegistry->getDefaultMaterialID()) {
                            neighborCell->material = materialRegistry->getFireID();
                            neighborCell->temperature = 500.0f;  // Hotter fire
                            neighborCell->setFlag(Cell::FLAG_BURNING);
                            neighborCell->lifetime = static_cast<uint8_t>(50 + getRandomInt(0, 20)); // Longer lasting fire
                            
                            // To make the fire more visibly active, give it an upward velocity
                            neighborCell->velocity.y = -0.5f; // Upward movement
                        }
                    } else {
                        // Try to add some smoke effects to empty cells
                        Cell* neighborCell = getAdjacentCell(cell1, 0, -1); // Try above first
                        if (neighborCell && neighborCell->material == materialRegistry->getDefaultMaterialID()) {
                            neighborCell->material = materialRegistry->getSmokeID();
                            neighborCell->temperature = 150.0f;  // Hotter smoke
                            neighborCell->lifetime = static_cast<uint8_t>(70 + getRandomInt(0, 20)); // Longer lasting smoke
                            
                            // To make the smoke rise more visibly
                            neighborCell->velocity.y = -0.3f; // Upward movement
                        }
                    }
                }
                
                // When wood is completely burned, it turns to fire and then quickly to ash
                if (cell1.health <= 0.0f) {
                    cell1.material = materialRegistry->getFireID();
                    cell1.temperature = 400.0f;
                    cell1.lifetime = static_cast<uint8_t>(25 + getRandomInt(-5, 5));
                }
                
                return true;
            }
        } 
        // Regular ignition for other materials
        else if (rollProbability(props1.flammability * deltaTime * ignitionMultiplier)) {
            // Oil ignites INSTANTLY and always (100% chance) when in contact with fire or lava
            if (cell1.material == materialRegistry->getOilID()) {
                cell1.material = materialRegistry->getOilFireID();
                cell1.temperature = std::max(cell1.temperature, 650.0f);
                // Make oil fire spread to nearby oil more aggressively
                cell1.temperature += 50.0f;  // Extra hot to ignite nearby oil
                cell1.energy += 50.0f;       // Extra energy for stronger fire
            } else {
                cell1.material = materialRegistry->getFireID();
                cell1.temperature = std::max(cell1.temperature, 500.0f);
            }
            
            cell1.setFlag(Cell::FLAG_BURNING);
            
            // Adjust lifetime based on material type
            float lifetimeScale = 1.0f;
            if (cell1.material == materialRegistry->getOilID()) {
                lifetimeScale = 2.0f;  // Oil burns longer
            }
            
            cell1.lifetime = static_cast<uint8_t>(props1.burnRate * 200.0f * lifetimeScale);
            
            // Generate smoke occasionally during ignition
            if (rollProbability(0.1f)) {
                cell1.temperature += 20.0f;  // Extra heat from combustion
            }
            
            return true;
        }
    }
    
    // Water extinguishes fire
    if (props1.type == MaterialType::FIRE && 
        props2.type == MaterialType::LIQUID && 
        props2.name.find("Water") != std::string::npos) {
        
        // Oil fire is harder to extinguish with water
        bool isOilFire = (cell1.material == materialRegistry->getOilFireID());
        float extinguishProbability = isOilFire ? 0.4f : 0.8f;
        
        if (rollProbability(extinguishProbability * deltaTime * 10.0f)) {
            // Convert to smoke
            cell1.material = materialRegistry->getSmokeID();
            cell1.clearFlag(Cell::FLAG_BURNING);
            
            // Oil fire produces more steam/smoke and hotter water
            if (isOilFire) {
                cell1.lifetime = 150; // More smoke from oil fire
                cell1.metadata = 1;   // Darker smoke
                cell2.temperature += 40.0f; // Water heats up more from oil fire
                
                // Small chance of steam generation from the hot water
                if (rollProbability(0.3f)) {
                    cell2.material = materialRegistry->getSteamID();
                    cell2.temperature = 110.0f;
                }
            } else {
                // Regular fire effects
                cell1.lifetime = 100;
                cell2.temperature += 20.0f; // Water heats up a bit
            }
            
            return true;
        }
    }
    else if (props2.type == MaterialType::FIRE && 
             props1.type == MaterialType::LIQUID && 
             props1.name.find("Water") != std::string::npos) {
        
        // Oil fire is harder to extinguish with water
        bool isOilFire = (cell2.material == materialRegistry->getOilFireID());
        float extinguishProbability = isOilFire ? 0.4f : 0.8f;
        
        if (rollProbability(extinguishProbability * deltaTime * 10.0f)) {
            // Convert to smoke
            cell2.material = materialRegistry->getSmokeID();
            cell2.clearFlag(Cell::FLAG_BURNING);
            
            // Oil fire produces more steam/smoke and hotter water
            if (isOilFire) {
                cell2.lifetime = 150; // More smoke from oil fire
                cell2.metadata = 1;   // Darker smoke
                cell1.temperature += 40.0f; // Water heats up more from oil fire
                
                // Small chance of steam generation from the hot water
                if (rollProbability(0.3f)) {
                    cell1.material = materialRegistry->getSteamID();
                    cell1.temperature = 110.0f;
                }
            } else {
                // Regular fire effects
                cell2.lifetime = 100;
                cell1.temperature += 20.0f; // Water heats up a bit
            }
            
            return true;
        }
    }
    
    // Acid dissolving materials
    if (props1.name.find("Acid") != std::string::npos && props2.type == MaterialType::SOLID) {
        if (rollProbability(0.2f * deltaTime * 5.0f)) {
            cell2.health -= 0.2f * deltaTime * 5.0f;
            if (cell2.health <= 0.0f) {
                cell2.material = materialRegistry->getDefaultMaterialID(); // Dissolved to nothing
            }
            return true;
        }
    }
    else if (props2.name.find("Acid") != std::string::npos && props1.type == MaterialType::SOLID) {
        if (rollProbability(0.2f * deltaTime * 5.0f)) {
            cell1.health -= 0.2f * deltaTime * 5.0f;
            if (cell1.health <= 0.0f) {
                cell1.material = materialRegistry->getDefaultMaterialID(); // Dissolved to nothing
            }
            return true;
        }
    }
    
    return false;
}

void CellProcessor::processStateChange(Cell& cell, float deltaTime)
{
    // Skip empty cells
    if (cell.material == materialRegistry->getDefaultMaterialID()) {
        return;
    }
    
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Process lifetime for temporary materials
    if (props.lifetime > 0.0f) {
        cell.lifetime = cell.lifetime > 0 ? cell.lifetime - 1 : 0;
        if (cell.lifetime == 0) {
            // Material has decayed, transform to its decay product
            if (props.type == MaterialType::FIRE) {
                cell.material = materialRegistry->getSmokeID();
                cell.temperature = std::max(100.0f, cell.temperature * 0.5f);
                cell.clearFlag(Cell::FLAG_BURNING);
            }
            else if (props.type == MaterialType::GAS) {
                cell.material = materialRegistry->getDefaultMaterialID(); // Gas dissipates
            }
            return;
        }
    }
    
    // Process temperature-based state changes
    for (const auto& stateChange : props.stateChanges) {
        bool conditionMet = false;
        
        // Check temperature threshold
        if (stateChange.temperatureThreshold > 0 && cell.temperature >= stateChange.temperatureThreshold) {
            // High temperature transition (melting, boiling, burning)
            conditionMet = true;
        }
        else if (stateChange.temperatureThreshold < 0 && cell.temperature <= -stateChange.temperatureThreshold) {
            // Low temperature transition (freezing, condensing)
            conditionMet = true;
        }
        
        // Apply probability check
        if (conditionMet && rollProbability(stateChange.probability * deltaTime * 5.0f)) {
            // Apply the state change
            MaterialID oldMaterial = cell.material;
            
            // Ensure material ID is valid before using it
            if (stateChange.targetMaterial > 0 && stateChange.targetMaterial <= 100) {
                // Valid ID - set it directly
                cell.material = stateChange.targetMaterial;
            } else {
                // Invalid ID - use Air instead
                cell.material = materialRegistry->getDefaultMaterialID(); // Air (0)
            }
            
            // Initialize the new material properties
            initializeCellFromMaterial(cell, cell.material);
            
            // Preserve appropriate flags
            if (props.type == MaterialType::FIRE) {
                cell.setFlag(Cell::FLAG_BURNING);
            }
            
            return;
        }
    }
}

void CellProcessor::transferHeat(Cell& sourceCell, Cell& targetCell, float deltaTime) const
{
    // Skip if cells are the same or if either is empty
    if (&sourceCell == &targetCell || 
        sourceCell.material == materialRegistry->getDefaultMaterialID() || 
        targetCell.material == materialRegistry->getDefaultMaterialID()) {
        return;
    }
    
    // Get material properties
    const MaterialProperties& sourceProps = materialRegistry->getMaterial(sourceCell.material);
    const MaterialProperties& targetProps = materialRegistry->getMaterial(targetCell.material);
    
    // Calculate temperature difference
    float tempDiff = sourceCell.temperature - targetCell.temperature;
    if (std::abs(tempDiff) < 0.1f) {
        return; // No significant temperature difference
    }
    
    // Calculate heat transfer rate based on material types
    // In our simplified system, we use material types to determine heat transfer
    float transferRate = 0.3f; // Default transfer rate
    
    // COMPLETELY DISABLED: Heat transfer multipliers were causing infinite ignition distance
    // Heat transfer should be kept at the default rate to prevent spreading across infinite distance
    
    // Heat transfer rate is now fixed at a very low value
    transferRate = 0.01f;
    
    // Prevent fire/heat from spreading across unlimited distance
    
    // Only allow direct contact heat transfer for fire
    if ((sourceProps.type == MaterialType::FIRE && 
         (targetProps.type == MaterialType::FIRE || targetProps.name == "Wood")) ||
        (targetProps.type == MaterialType::FIRE && 
         (sourceProps.type == MaterialType::FIRE || sourceProps.name == "Wood"))) {
        // Only allow heat transfer for direct contact between fire and wood
        transferRate = 0.1f;
    } else {
        // For all other materials, make heat transfer extremely slow
        transferRate = 0.001f;
    }
    
    // DISABLED: Heat transfer between wood pieces was causing infinite distance ignition
    // Wood fire should only spread by direct contact, not through heat transfer
    /*
    if (sourceProps.name == "Wood" && targetProps.name == "Wood" && 
        sourceCell.hasFlag(Cell::FLAG_BURNING)) {
        // This logic caused infinite distance fire ignition
        // and has been completely disabled
    }
    */
    
    // Scale by delta time for consistent behavior
    float transfer = tempDiff * transferRate * deltaTime * 0.1f;
    
    // Apply transfer with simple adjustment based on material type
    float sourceHeatCapacity = 1.0f; // Default heat capacity
    float targetHeatCapacity = 1.0f;
    
    // Liquids and solids have higher heat capacity
    if (sourceProps.type == MaterialType::LIQUID || sourceProps.type == MaterialType::SOLID) {
        sourceHeatCapacity = 2.0f;
    }
    if (targetProps.type == MaterialType::LIQUID || targetProps.type == MaterialType::SOLID) {
        targetHeatCapacity = 2.0f;
    }
    
    // Apply the heat transfer
    sourceCell.temperature -= transfer / sourceHeatCapacity;
    targetCell.temperature += transfer / targetHeatCapacity;
    
    // COMPLETELY DISABLED: Smoke generation from distant burning materials
    // This was causing heat to propagate and affect materials at large distances
    /*
    if (sourceProps.type == MaterialType::FIRE && targetProps.flammable && 
        targetProps.type != MaterialType::FIRE) {
        // Disabled - this was causing problems with infinite distance heat propagation
    }
    */
}

bool CellProcessor::checkStateChangeByTemperature(Cell& cell)
{
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Check melting point (solid to liquid)
    if (props.type == MaterialType::SOLID && props.meltingPoint > 0 && 
        cell.temperature >= props.meltingPoint) {
        
        // Find appropriate liquid form of this material
        for (const auto& stateChange : props.stateChanges) {
            const MaterialProperties& targetProps = materialRegistry->getMaterial(stateChange.targetMaterial);
            if (targetProps.type == MaterialType::LIQUID) {
                cell.material = stateChange.targetMaterial;
                return true;
            }
        }
    }
    
    // Check freezing point (liquid to solid)
    if (props.type == MaterialType::LIQUID && props.freezingPoint > 0 && 
        cell.temperature <= props.freezingPoint) {
        
        // Find appropriate solid form of this material
        for (const auto& stateChange : props.stateChanges) {
            const MaterialProperties& targetProps = materialRegistry->getMaterial(stateChange.targetMaterial);
            if (targetProps.type == MaterialType::SOLID) {
                cell.material = stateChange.targetMaterial;
                return true;
            }
        }
    }
    
    // Check boiling point (liquid to gas)
    if (props.type == MaterialType::LIQUID && props.boilingPoint > 0 && 
        cell.temperature >= props.boilingPoint) {
        
        // Find appropriate gas form of this material
        for (const auto& stateChange : props.stateChanges) {
            const MaterialProperties& targetProps = materialRegistry->getMaterial(stateChange.targetMaterial);
            if (targetProps.type == MaterialType::GAS) {
                cell.material = stateChange.targetMaterial;
                return true;
            }
        }
    }
    
    // Check condensation (gas to liquid)
    if (props.type == MaterialType::GAS && props.boilingPoint > 0 && 
        cell.temperature < props.boilingPoint - 5.0f) {
        
        // Find appropriate liquid form of this material
        for (const auto& stateChange : props.stateChanges) {
            const MaterialProperties& targetProps = materialRegistry->getMaterial(stateChange.targetMaterial);
            if (targetProps.type == MaterialType::LIQUID) {
                cell.material = stateChange.targetMaterial;
                return true;
            }
        }
    }
    
    // Check ignition point for flammable materials
    if (props.flammable && props.ignitionPoint > 0 && 
        cell.temperature >= props.ignitionPoint) {
        
        // Only convert to fire if material is not wood
        if (props.name != "Wood" && cell.material != materialRegistry->getWoodID()) {
            // Set burning flag
            cell.setFlag(Cell::FLAG_BURNING);
            
            // For oil, use oil fire
            if (props.name == "Oil" || cell.material == materialRegistry->getOilID()) {
                cell.material = materialRegistry->getOilFireID();
            } else {
                // Verify material ID is valid
                MaterialID fireID = materialRegistry->getFireID();
                if (fireID > 0 && fireID <= 100) {
                    cell.material = fireID;
                }
                // If Fire ID invalid, just keep existing material with the burning flag
            }
            
            cell.temperature = std::max(500.0f, cell.temperature);
            cell.lifetime = static_cast<uint8_t>(props.burnRate * 200.0f);
        } else {
            // Wood gets burning flag only, it stays as wood
            cell.setFlag(Cell::FLAG_BURNING);
        }
        return true;
    }
    
    return false;
}

void CellProcessor::applyVelocity(Cell& cell, const glm::vec2& direction, float speed)
{
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Adjust speed based on material properties
    float adjustedSpeed = speed;
    
    // Solids have resistance to movement
    if (props.type == MaterialType::SOLID) {
        adjustedSpeed *= (1.0f - props.friction);
    }
    // Powders have moderate resistance
    else if (props.type == MaterialType::POWDER) {
        adjustedSpeed *= (1.0f - props.friction * 0.5f);
    }
    // Liquids flow with viscosity affecting speed
    else if (props.type == MaterialType::LIQUID) {
        adjustedSpeed *= (1.0f - props.viscosity);
    }
    // Gases move easily but are affected by dispersion
    else if (props.type == MaterialType::GAS) {
        adjustedSpeed *= props.dispersion;
    }
    
    // Set the new velocity (we can add to existing or replace)
    glm::vec2 normalizedDir = glm::length(direction) > 0.0f ? 
                              glm::normalize(direction) : 
                              glm::vec2(0.0f, 0.0f);
    
    cell.velocity = normalizedDir * adjustedSpeed;
}

void CellProcessor::applyPressure(Cell& cell, float amount)
{
    // Only apply pressure to materials that can be pressurized
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (props.type == MaterialType::LIQUID || props.type == MaterialType::GAS) {
        cell.pressure += amount;
        
        // Set pressurized flag if pressure is high
        if (cell.pressure > 5.0f) {
            cell.setFlag(Cell::FLAG_PRESSURIZED);
        } else {
            cell.clearFlag(Cell::FLAG_PRESSURIZED);
        }
        
        // High pressure can increase temperature slightly
        if (cell.pressure > 10.0f) {
            cell.temperature += amount * 0.1f;
        }
    }
}

void CellProcessor::damageCell(Cell& cell, float amount)
{
    // Apply damage to cell health
    cell.health = std::max(0.0f, cell.health - amount);
    
    // If health is depleted, handle destruction
    if (cell.health <= 0.0f) {
        const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
        
        // Different materials break/destroy differently
        switch (props.type) {
            case MaterialType::SOLID:
                // Solids can break into powders or disappear
                if (props.name.find("Stone") != std::string::npos || 
                    props.name.find("Rock") != std::string::npos) {
                    cell.material = materialRegistry->getSandID(); // Rock breaks to sand
                } else {
                    cell.material = materialRegistry->getDefaultMaterialID(); // Disappear
                }
                break;
                
            case MaterialType::POWDER:
                // Powders can compact or disperse
                cell.material = materialRegistry->getDefaultMaterialID();
                break;
                
            case MaterialType::LIQUID:
                // Liquids typically evaporate
                cell.material = materialRegistry->getDefaultMaterialID();
                break;
                
            case MaterialType::GAS:
                // Gases dissipate
                cell.material = materialRegistry->getDefaultMaterialID();
                break;
                
            case MaterialType::FIRE:
                // Fire extinguishes
                cell.material = materialRegistry->getSmokeID();
                cell.clearFlag(Cell::FLAG_BURNING);
                break;
                
            default:
                cell.material = materialRegistry->getDefaultMaterialID();
                break;
        }
        
        // Reset cell properties
        cell.health = 1.0f;
        cell.velocity = glm::vec2(0.0f, 0.0f);
        cell.pressure = 0.0f;
    }
}

void CellProcessor::igniteCell(Cell& cell)
{
    // Check if material is flammable
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (props.flammable) {
        // Convert to fire based on material type
        MaterialID oldMaterial = cell.material;
        
        // Handle wood specially - wood stays as wood but gets the burning flag
        if (oldMaterial == materialRegistry->getWoodID() || props.name == "Wood") {
            // Just set the burning flag, don't change material
            cell.setFlag(Cell::FLAG_BURNING);
            std::cout << "WOOD IGNITED: Burning flag set, material unchanged" << std::endl;
            return;
        }
        
        // Special case for oil - it creates oil fire
        if (oldMaterial == materialRegistry->getOilID() || props.name == "Oil") {
            std::cout << "OIL IGNITED: Converting to oil fire" << std::endl;
            cell.material = materialRegistry->getOilFireID();
        } else {
            // Regular fire for everything else - ensure ID is valid first
            MaterialID fireID = materialRegistry->getFireID();
            if (fireID > 0 && fireID <= 100) {
                cell.material = fireID;
            } else {
                // If fire ID is invalid, keep original material
                cell.material = oldMaterial;
            }
        }
        
        cell.setFlag(Cell::FLAG_BURNING);
        
        // Fire lifetime based on burn rate of original material
        float lifetimeScale = 1.0f;
        if (oldMaterial == materialRegistry->getOilID()) {
            lifetimeScale = 2.0f;  // Oil burns longer
        }
        
        cell.lifetime = static_cast<uint8_t>(props.burnRate * 200.0f * lifetimeScale);
        
        // Fire energy based on flammability
        cell.energy = props.flammability * 100.0f;
        
        // Smoke production starts with the ignition
        if (getRandomFloat(0.0f, 1.0f) < 0.2f) {
            // Occasionally spawn smoke particles above the fire
            cell.temperature += 20.0f;  // Extra heat boost from the combustion
        }
    }
    else if (props.type != MaterialType::FIRE) {
        // If not flammable, just heat it up
        cell.temperature += 150.0f;  // Increased heat boost
    }
}

void CellProcessor::extinguishCell(Cell& cell)
{
    // Check if cell is on fire
    if (cell.hasFlag(Cell::FLAG_BURNING) || 
        materialRegistry->getMaterial(cell.material).type == MaterialType::FIRE) {
        
        // Convert fire to smoke
        cell.material = materialRegistry->getSmokeID();
        cell.clearFlag(Cell::FLAG_BURNING);
        
        // Base temperature and lifetime for smoke
        float smokeTemp = 100.0f;
        uint8_t smokeLifetime = 100;
        
        // Oil fires produce denser, hotter smoke that lasts longer
        if (cell.material == materialRegistry->getOilFireID()) {
            smokeTemp = 130.0f;
            smokeLifetime = 150;
            
            // Darker smoke for oil fires - use metadata to indicate oil smoke
            cell.metadata = 1;
        }
        
        cell.temperature = std::min(cell.temperature, smokeTemp);
        cell.lifetime = smokeLifetime;
    }
}

void CellProcessor::freezeCell(Cell& cell)
{
    // Check if material can freeze
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (props.type == MaterialType::LIQUID && props.freezingPoint > 0) {
        // Find an appropriate solid version
        for (const auto& stateChange : props.stateChanges) {
            if (stateChange.temperatureThreshold < 0) { // Negative threshold for freezing
                const MaterialProperties& targetProps = materialRegistry->getMaterial(stateChange.targetMaterial);
                if (targetProps.type == MaterialType::SOLID) {
                    // Apply the freeze
                    cell.material = stateChange.targetMaterial;
                    cell.temperature = props.freezingPoint - 5.0f;
                    cell.setFlag(Cell::FLAG_FROZEN);
                    return;
                }
            }
        }
        
        // If no specific transition, just reduce temperature and set flag
        cell.temperature = 0.0f;
        cell.setFlag(Cell::FLAG_FROZEN);
    }
    else {
        // Just cool the cell down
        cell.temperature = std::max(cell.temperature - 50.0f, -10.0f);
    }
}

void CellProcessor::meltCell(Cell& cell)
{
    // Check if material can melt
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (props.type == MaterialType::SOLID && props.meltingPoint > 0) {
        // Find an appropriate liquid version
        for (const auto& stateChange : props.stateChanges) {
            if (stateChange.temperatureThreshold > 0) { // Positive threshold for melting
                const MaterialProperties& targetProps = materialRegistry->getMaterial(stateChange.targetMaterial);
                if (targetProps.type == MaterialType::LIQUID) {
                    // Apply the melt
                    cell.material = stateChange.targetMaterial;
                    cell.temperature = props.meltingPoint + 5.0f;
                    cell.clearFlag(Cell::FLAG_FROZEN);
                    return;
                }
            }
        }
        
        // If no specific transition, just increase temperature
        cell.temperature = props.meltingPoint + 5.0f;
        cell.clearFlag(Cell::FLAG_FROZEN);
    }
    else {
        // Just heat the cell up
        cell.temperature += 50.0f;
    }
}

void CellProcessor::dissolveCell(Cell& cell, float rate)
{
    // Check if material can dissolve - in our simplified system, we use CORROSIVE flag
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Use corrosive flag instead of dissolves property
    if (props.hasFlag(MaterialProperties::Flags::CORROSIVE)) {
        cell.setFlag(Cell::FLAG_DISSOLVING);
        
        // Apply damage at the given rate
        cell.health -= rate;
        
        // Check if completely dissolved
        if (cell.health <= 0.0f) {
            cell.material = materialRegistry->getDefaultMaterialID();
            cell.health = 1.0f;
            cell.clearFlag(Cell::FLAG_DISSOLVING);
        }
    }
}

Cell* CellProcessor::getAdjacentCell(const Cell& cell, int dx, int dy)
{
    // This is a placeholder implementation because we don't have access to the world grid here
    // In a real implementation, we would need to get the coordinates of the current cell
    // and return a pointer to the adjacent cell at (x+dx, y+dy)
    
    // Since we don't have direct access to the world coordinates or grid from the CellProcessor,
    // we'll use a dummy approach just to make the code compile
    
    // CRITICAL: Use a NEW dummyCell each time to avoid memory corruption
    // This prevents issues with invalid material IDs
    static Cell dummyCell1; // For up direction
    static Cell dummyCell2; // For down direction
    static Cell dummyCell3; // For left direction
    static Cell dummyCell4; // For right direction
    
    // We'll return null for diagonal cells
    if (dx != 0 && dy != 0) {
        return nullptr;
    }
    
    // Only allow cardinal directions
    // Use a different static cell for each direction to prevent corruption
    if (dy < 0 && dx == 0) { // Up
        // Reset cell to valid state
        dummyCell1 = Cell(); // Reset to default state with valid IDs
        dummyCell1.material = materialRegistry->getDefaultMaterialID(); // Air
        return &dummyCell1;
    } 
    else if (dy > 0 && dx == 0) { // Down
        dummyCell2 = Cell(); // Reset completely
        dummyCell2.material = materialRegistry->getDefaultMaterialID();
        return &dummyCell2;
    }
    else if (dx < 0 && dy == 0) { // Left
        dummyCell3 = Cell(); // Reset completely
        dummyCell3.material = materialRegistry->getDefaultMaterialID();
        return &dummyCell3;
    }
    else if (dx > 0 && dy == 0) { // Right
        dummyCell4 = Cell(); // Reset completely
        dummyCell4.material = materialRegistry->getDefaultMaterialID();
        return &dummyCell4;
    }
    
    // Otherwise return null
    return nullptr;
}

float CellProcessor::getRandomFloat(float min, float max) const
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(random);
}

int CellProcessor::getRandomInt(int min, int max) const
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(random);
}

bool CellProcessor::rollProbability(float chance) const
{
    // Ensure chance is between 0 and 1
    chance = std::max(0.0f, std::min(1.0f, chance));
    return getRandomFloat(0.0f, 1.0f) < chance;
}

} // namespace astral