// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ArenaBattle.h"
#include "GameFramework/Actor.h"
#include "Fountain.generated.h"


UCLASS()
class ARENABATTLE_API AFountain : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFountain();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Body;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Water;

	UPROPERTY(VisibleAnywhere)
	UPointLightComponent* Light;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* Splash;
	
	UPROPERTY(EditAnywhere, Category = ID)
	int32 ID;

public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;

private:
	// ������ ����� ������
	// UPROPERTY��ũ�ο� AllowPrivateAccess��� METAŰ���� �߰��ϸ� �����Ϳ����� �̸� �����԰� ���ÿ� ���� �����͸� ���а��� => ĸ��ȭ
	UPROPERTY(EditAnywhere, Category = Stat, Meta = (AllowPrivateAccess = true))
	float RotateSpeed = 30.f;

	// Movement Comp �׽�Ʈ
public:
	UPROPERTY(VisibleAnywhere)
	class URotatingMovementComponent* Movement;
};
