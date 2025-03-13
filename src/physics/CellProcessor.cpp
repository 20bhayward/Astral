#include "astral/physics/CellularPhysics.h"
#include "astral/physics/Material.h"
#include <chrono>
#include <algorithm>
#include <cmath>

namespace astral {

CellProcessor::CellProcessor(MaterialRegistry* registry)
    : materialRegistry(registry)
{
    // Initialize random number generator with current time
    random.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
}

void CellProcessor::initializeCellFromMaterial(Cell& cell, MaterialID materialID) const
{
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
            cell.temperature = 500.0f; // High starting temperature
            cell.lifetime = static_cast<uint8_t>(props.lifetime > 0 ? props.lifetime : 100);
            cell.setFlag(Cell::FLAG_BURNING);
            cell.energy = 100.0f; // Fire has high energy
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
    if (props.conductive) {
        cell.charge = props.conductivity * 10.0f; // Store conductivity as potential charge
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
    static int processorCallCount = 0;
    processorCallCount++;
    bool shouldLog = (processorCallCount % 10000 == 0);
    
    // Cannot move if the cell is not movable
    const MaterialProperties& cellProps = materialRegistry->getMaterial(cell.material);
    if (!cellProps.movable) {
        if (shouldLog) {
            std::cout << "PROCESSOR #" << processorCallCount << ": Movement blocked - " 
                      << cellProps.name << " is not movable!" << std::endl;
        }
        return false;
    }
    
    // Can always move into empty space
    if (target.material == materialRegistry->getDefaultMaterialID()) {
        if (shouldLog) {
            std::cout << "PROCESSOR #" << processorCallCount << ": APPROVED - Moving into empty space" << std::endl;
        }
        return true;
    }
    
    // Check if densities allow displacement
    const MaterialProperties& targetProps = materialRegistry->getMaterial(target.material);
    
    // CRITICAL FIX: Allow any cell to move into air spaces
    if (targetProps.type == MaterialType::EMPTY) {
        if (shouldLog) {
            std::cout << "PROCESSOR #" << processorCallCount << ": APPROVED - Moving into empty type" << std::endl;
        }
        return true; 
    }
    
    // Liquids and powders can displace less dense materials
    if ((cellProps.type == MaterialType::LIQUID || cellProps.type == MaterialType::POWDER) &&
        cellProps.density > targetProps.density) {
        if (shouldLog) {
            std::cout << "PROCESSOR #" << processorCallCount << ": APPROVED - Denser " 
                      << cellProps.name << " displacing less dense " << targetProps.name << std::endl;
        }
        return true;
    }
    
    // Gases can displace other gases if they're denser
    if (cellProps.type == MaterialType::GAS && targetProps.type == MaterialType::GAS &&
        cellProps.density > targetProps.density) {
        if (shouldLog) {
            std::cout << "PROCESSOR #" << processorCallCount << ": APPROVED - Denser gas displacement" << std::endl;
        }
        return true;
    }
    
    if (shouldLog) {
        std::cout << "PROCESSOR #" << processorCallCount << ": BLOCKED - No displacement rule matched: " 
                  << cellProps.name << " (type=" << (int)cellProps.type << ", density=" << cellProps.density 
                  << ") vs " << targetProps.name << " (type=" << (int)targetProps.type 
                  << ", density=" << targetProps.density << ")" << std::endl;
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
    
    // Check for direct reaction rules in material properties
    for (const auto& reaction : props1.reactions) {
        if (reaction.reactantMaterial == cell2.material) {
            // Apply probability check
            if (rollProbability(reaction.probability * deltaTime * 10.0f)) {
                // Perform the reaction
                cell1.material = reaction.resultMaterial;
                cell1.energy += reaction.energyRelease;
                cell1.temperature += reaction.energyRelease * 10.0f;
                
                // If energy release is significant, set cell on fire
                if (reaction.energyRelease > 5.0f) {
                    cell1.setFlag(Cell::FLAG_BURNING);
                }
                
                return true;
            }
        }
    }
    
    // Special case: Fire and flammable materials
    if (props1.type == MaterialType::FIRE && props2.flammable) {
        if (rollProbability(props2.flammability * deltaTime * 5.0f)) {
            cell2.material = materialRegistry->getFireID();
            cell2.temperature = std::max(cell2.temperature, 500.0f);
            cell2.setFlag(Cell::FLAG_BURNING);
            cell2.lifetime = static_cast<uint8_t>(props2.burnRate * 200.0f);
            return true;
        }
    }
    else if (props2.type == MaterialType::FIRE && props1.flammable) {
        if (rollProbability(props1.flammability * deltaTime * 5.0f)) {
            cell1.material = materialRegistry->getFireID();
            cell1.temperature = std::max(cell1.temperature, 500.0f);
            cell1.setFlag(Cell::FLAG_BURNING);
            cell1.lifetime = static_cast<uint8_t>(props1.burnRate * 200.0f);
            return true;
        }
    }
    
    // Water extinguishes fire
    if (props1.type == MaterialType::FIRE && 
        props2.type == MaterialType::LIQUID && 
        props2.name.find("Water") != std::string::npos) {
        
        if (rollProbability(0.8f * deltaTime * 10.0f)) {
            cell1.material = materialRegistry->getSmokeID();
            cell1.clearFlag(Cell::FLAG_BURNING);
            cell2.temperature += 20.0f; // Water heats up a bit
            return true;
        }
    }
    else if (props2.type == MaterialType::FIRE && 
             props1.type == MaterialType::LIQUID && 
             props1.name.find("Water") != std::string::npos) {
        
        if (rollProbability(0.8f * deltaTime * 10.0f)) {
            cell2.material = materialRegistry->getSmokeID();
            cell2.clearFlag(Cell::FLAG_BURNING);
            cell1.temperature += 20.0f; // Water heats up a bit
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
            cell.material = stateChange.targetMaterial;
            
            // Initialize the new material properties
            initializeCellFromMaterial(cell, cell.material);
            
            // Preserve temperature across state change
            cell.temperature = props.type == MaterialType::FIRE ? 500.0f : cell.temperature;
            
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
    
    // Calculate heat transfer rate based on thermal conductivity
    float transferRate = std::min(sourceProps.thermalConductivity, targetProps.thermalConductivity);
    transferRate = std::max(0.01f, transferRate); // Ensure minimum conductivity
    
    // Scale by delta time for consistent behavior
    float transfer = tempDiff * transferRate * deltaTime * 0.1f;
    
    // Apply transfer with respect to specific heat
    sourceCell.temperature -= transfer / std::max(0.1f, sourceProps.specificHeat);
    targetCell.temperature += transfer / std::max(0.1f, targetProps.specificHeat);
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
        
        // Convert to fire
        cell.material = materialRegistry->getFireID();
        cell.temperature = std::max(500.0f, cell.temperature);
        cell.setFlag(Cell::FLAG_BURNING);
        cell.lifetime = static_cast<uint8_t>(props.burnRate * 200.0f);
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
        // Convert to fire
        MaterialID oldMaterial = cell.material;
        cell.material = materialRegistry->getFireID();
        cell.temperature = std::max(500.0f, cell.temperature);
        cell.setFlag(Cell::FLAG_BURNING);
        
        // Fire lifetime based on burn rate of original material
        cell.lifetime = static_cast<uint8_t>(props.burnRate * 200.0f);
        
        // Fire energy based on flammability
        cell.energy = props.flammability * 100.0f;
    }
    else if (props.type != MaterialType::FIRE) {
        // If not flammable, just heat it up
        cell.temperature += 100.0f;
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
        cell.temperature = std::min(cell.temperature, 100.0f);
        cell.lifetime = 100; // Smoke lasts a while
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
    // Check if material can dissolve
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (props.dissolves) {
        cell.setFlag(Cell::FLAG_DISSOLVING);
        
        // Apply damage at the dissolution rate
        float dissolutionRate = props.dissolutionRate > 0.0f ? props.dissolutionRate : rate;
        cell.health -= dissolutionRate;
        
        // Check if completely dissolved
        if (cell.health <= 0.0f) {
            cell.material = materialRegistry->getDefaultMaterialID();
            cell.health = 1.0f;
            cell.clearFlag(Cell::FLAG_DISSOLVING);
        }
    }
}

float CellProcessor::getRandomFloat(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(random);
}

int CellProcessor::getRandomInt(int min, int max)
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(random);
}

bool CellProcessor::rollProbability(float chance)
{
    // Ensure chance is between 0 and 1
    chance = std::max(0.0f, std::min(1.0f, chance));
    return getRandomFloat(0.0f, 1.0f) < chance;
}

} // namespace astral